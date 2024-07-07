/**
 * @file FlightCore.cc
 * @brief Main control loop for the flyMS program
 *
 * @author Mike Sardonini
 * @date 10/15/2018
 */

#include "flyMS/FlightCore.h"

#include <sys/stat.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "flyMS/common/constants.h"
#include "flyMS/hardware/RemoteController.h"
#include "flyMS/util/debug_mode.h"
#include "rc/start_stop.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace flyMS {

FlightCore::FlightCore(Setpoint &&setpoint, PositionController &&position_controller, const YAML::Node &config_params,
                       std::shared_ptr<PruRequester> pru_requester)
    : flight_mode_(static_cast<FlightMode>(config_params["flight_mode"].as<uint32_t>())),
      config_params_(config_params),
      pru_requester_(pru_requester),
      imu_module_(Imu::getInstance()),
      ulog_(),
      setpoint_module_(std::move(setpoint)),
      attitude_controller_(config_params["controller"]),
      position_controller_(std::move(position_controller)) {
  log_filepath_ = config_params["log_filepath"].as<std::string>();
}

FlightCore::FlightCore(const YAML::Node &config_params, std::shared_ptr<PruRequester> pru_requester)
    : FlightCore(
          Setpoint(static_cast<FlightMode>(config_params["flight_mode"].as<uint32_t>()), config_params["setpoint"]),
          PositionController(config_params["position_controller"]), config_params, pru_requester) {}

bool FlightCore::init() {
  // Create a file for logging and initialize our file logger
  init_logging(log_filepath_);

  // TODO: improve the logic to decide whether to initialize the object or not
  if (config_params_["mavlink_interface"]["enable"].as<bool>()) {
    mavlink_subscriber_ = std::make_unique<MavlinkRedisSubQueue>(kFLY_STEREO_CHANNEL);
    odometry_queue_ = mavlink_subscriber_->register_message_with_queue<mavlink_odometry_t, MAVLINK_MSG_ID_ODOMETRY>(
        &mavlink_msg_odometry_decode);
    mavlink_publisher_ = std::make_unique<MavlinkRedisPub>();
  }

  // Blocks execution until a packet is received
  if constexpr (!kDEBUG_MODE) {
    auto &remote = RemoteController::get_instance();
    remote.wait_for_data_packet();

    // Command the state variable be set to EXITING if the RC loses connection
    remote.packet_loss_callback([]() { rc_set_state(EXITING); });
  }
  // Tell the system that we are running if we are in the predicted UNINITIALIZED state. Else shutdown
  if (rc_get_state() == UNINITIALIZED) {
    rc_set_state(RUNNING);
  } else {
    rc_set_state(EXITING);
  }

  YAML::Node filters = config_params_["filters"];
  auto lpf_num = filters["imu_lpf_num"].as<std::vector<double>>();
  auto lpf_den = filters["imu_lpf_den"].as<std::vector<double>>();
  gyro_lpf_roll_ = DigitalFilter(lpf_num, lpf_den);
  gyro_lpf_pitch_ = DigitalFilter(lpf_num, lpf_den);
  gyro_lpf_yaw_ = DigitalFilter(lpf_num, lpf_den);

  // Start the flight program
  std::function<void(StateData &)> func = std::bind(&FlightCore::flight_core, this, std::placeholders::_1);
  imu_module_.init(config_params_["imu_params"], kFLYMS_CONTROL_LOOP_FREQUENCY, func);

  return true;
}

void FlightCore::flight_core(StateData &imu_data_body) {
  // Apply the low pass filters on the gyroscope data
  imu_data_body.eulerRate[0] = gyro_lpf_roll_.update_filter(imu_data_body.eulerRate[0]);
  imu_data_body.eulerRate[1] = gyro_lpf_pitch_.update_filter(imu_data_body.eulerRate[1]);
  imu_data_body.eulerRate[2] = gyro_lpf_yaw_.update_filter(imu_data_body.eulerRate[2]);

  // Get Remote Controller Data
  auto remote_data = RemoteController::get_instance().get_channel_data();

  if (remote_data.empty()) {
    if constexpr (kDEBUG_MODE) {  // In Debug mode this is OK
      remote_data.resize(4, 0.f);
    } else {
      throw std::runtime_error("No RC data received! Should have data by now");
    }
  }

  // Calculate controller setpoints
  auto setpoints = setpoint_module_.calculate_setpoint_data(remote_data);

  // Override setpoints with Position controller if enabled
  if (flyStereo_running_ && !odometry_queue_->empty()) {
    if (flyStereo_streaming_data_ == false) {
      standing_throttle_ = setpoints.throttle();
      flyStereo_streaming_data_ = true;
    }

    auto odom_data = odometry_queue_->pop_front();

    position_controller_.ReceiveVio(odom_data);
    auto [setpoint, yaw_vio] = position_controller_.GetSetpoint();

    // Only set the throttle for now
    setpoints.throttle() = setpoint(2);

    // print some debug mesasges for now
    spdlog::info("odom data: x: {:2.2f}, y: {:2.2f}, z: {:2.2f}", odom_data.x, odom_data.y, odom_data.z);
    spdlog::info("setpoint: x: {:2.2f}, y: {:2.2f}, z: {:2.2f}", setpoint(0), setpoint(1), setpoint(2));
  }

  // If landed for 2 seconds, reset integrators and Yaw error
  if (remote_data[kFLYMS_THROTTLE_INDEX] < -0.98) {
    integrator_reset_++;
  } else {
    integrator_reset_ = 0;
  }
  if (integrator_reset_ > 2 / kFLYMS_CONTROL_LOOP_DT) {
    spdlog::info("Land detected, resetting integrators at time {}", imu_data_body.timestamp_us);

    setpoints.yaw() = imu_data_body.euler[2];
    setpoint_module_.set_yaw_ref(imu_data_body.euler[2]);
    attitude_controller_.zero_pids();
  }

  // Calculate control loops to determine control effort for each Degree of Freedom
  auto att_ctrl = attitude_controller_.calculate_control_loop(setpoints, imu_data_body, flight_mode_);

  /************************************************************************
   *  Mixing:
   *          CCW 1    2 CW       Body Frame:
   *                \ /               X
   *                / \             Y_|
   *            CW 3   4 CCW              Z UP
   *
   ************************************************************************/
  // The amount of power (0 - 1) to give to each motor
  std::vector<float> u(4);
  u[0] = att_ctrl.throttle() + att_ctrl.roll() - att_ctrl.pitch() - att_ctrl.yaw();
  u[1] = att_ctrl.throttle() - att_ctrl.roll() - att_ctrl.pitch() + att_ctrl.yaw();
  u[2] = att_ctrl.throttle() + att_ctrl.roll() + att_ctrl.pitch() + att_ctrl.yaw();
  u[3] = att_ctrl.throttle() - att_ctrl.roll() + att_ctrl.pitch() - att_ctrl.yaw();

  // Check Output Ranges, if outside, adjust
  output_saturation_filter(u);

  // Send commands to motors/ESCs
  if constexpr (!kDEBUG_MODE) {
    pru_requester_->send_command(u);
  }

  // Check the kill Switch and Shutdown if set
  if constexpr (!kDEBUG_MODE) {
    if (remote_data[kFLYMS_KILL_SWITCH_INDEX] < 0.5) {
      spdlog::info("\nKill Switch Hit! Shutting Down\n");
      rc_set_state(EXITING);
    }
  }

  // Check if the flight mode has changed
  if (!flyStereo_running_ && remote_data[kFLYMS_FLIGHT_MODE_INDEX] > 0.5) {
    spdlog::info("Turning on flyStereo!");
    flyStereo_running_ = true;

    mavlink_command_int_t command_int;
    command_int.command = MAV_CMD_NAV_GUIDED_ENABLE;
    mavlink_publisher_->publish<mavlink_command_int_t>(kFLY_STEREO_CHANNEL, command_int,
                                                       &mavlink_msg_command_int_encode);
  } else if (flyStereo_running_ && remote_data[kFLYMS_FLIGHT_MODE_INDEX] < 0.5) {
    spdlog::info("Turning off flyStereo!");
    flyStereo_running_ = false;
    flyStereo_streaming_data_ = false;

    mavlink_command_int_t command_int;
    command_int.command = MAV_CMD_NAV_RETURN_TO_LAUNCH;
    mavlink_publisher_->publish<mavlink_command_int_t>(kFLY_STEREO_CHANNEL, command_int,
                                                       &mavlink_msg_command_int_encode);
  }

  // Print some stuff to the console in debug mode
  if constexpr (kDEBUG_MODE) {
    console_print(imu_data_body, setpoints.vector(), u);
  }

  // Log Important Flight Data For Analysis
  struct ULogFlightMsg flight_msg {
    imu_data_body.timestamp_us, imu_data_body, setpoints.vector(), u, att_ctrl.vector()
  };
  ulog_.write_msg(flight_msg);
}

void FlightCore::output_saturation_filter(std::vector<float> &u) {
  // Check Output Ranges, if outside, adjust
  static constexpr float smallest_value = 0.f;
  float largest_value = 1.f;
  for (int i = 0; i < 4; i++) {
    if (u[i] > largest_value) {
      largest_value = u[i];
    }
    if (u[i] < smallest_value) {
      u[i] = 0;
    }
  }

  // if upper saturation would have occurred, reduce all outputs evenly
  if (largest_value > 1) {
    float offset = largest_value - 1;
    for (int i = 0; i < 4; i++) {
      u[i] -= offset;
    }
  }
}

void FlightCore::console_print(const StateData &imu_data_body, const std::vector<float> &setpoints,
                               const std::vector<float> &u) {
  spdlog::debug(" Throt {:2.2f}, Roll_ref {:2.2f}, Pitch_ref {:2.2f}, Yaw_ref {:2.2f} ",
                setpoints[kFLYMS_THROTTLE_INDEX], setpoints[kFLYMS_ROLL_INDEX], setpoints[kFLYMS_PITCH_INDEX],
                setpoints[kFLYMS_YAW_INDEX]);
  spdlog::debug(" Motor 1: {:2.2f}, 2: {:2.2f}, 3: {:2.2f}, 4: {:2.2f}", u[0], u[1], u[2], u[3]);
  spdlog::debug(" Roll {:2.2f}, Pitch {:2.2f}, Yaw {:2.2f}", imu_data_body.euler[0], imu_data_body.euler[1],
                imu_data_body.euler[2]);
}

void FlightCore::init_logging(const std::filesystem::path &log_location) {
  auto log_dir = ULog::generate_incremented_run_dir(log_location);

  // Initialize spdlog
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  int max_bytes = 1048576 * 20;  // Max 20 MB
  int max_files = 20;
  sinks.push_back(
      std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_dir / "console_log.txt", max_bytes, max_files));
  auto flyMS_log = std::make_shared<spdlog::logger>("flyMS_log", std::begin(sinks), std::end(sinks));

  // Register loggers to be global logger
  flyMS_log->set_level(spdlog::level::trace);
  spdlog::register_logger(flyMS_log);
  spdlog::set_default_logger(flyMS_log);

  // Initialize Ulog
  ulog_.init(log_dir / "logger.ulg");
}

}  // namespace flyMS
