/**
 * @file FlightCore.h
 * @brief Header for the FlightCore object
 *
 * @author Mike Sardonini
 * @date 10/15/2018
 */
#pragma once

// TODO let flyMS run as non-root and access the pru
// https://unix.stackexchange.com/questions/475800/non-root-read-access-to-dev-mem-by-kmem-group-members-fails
// TODO handle the flight mode config parameter appropriately
// TODO Consistent Redis hostname from both x86 and arm
// TODO add a rotation matrix config field to update with the webserver
// TODO find any other config parameters that need to be adjustable via the web interface
// TODO expand unit tests, automate with github pipelines
// TODO deploy webserver UI to github pages
// TODO generate logo using DALL-E
// TODO document the following in sphinx:
// 1. Interfacing with the flyMS webserver

// System Includes
#include <array>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "flyMS/common/flight_mode.h"
#include "flyMS/common/state_data.h"
#include "flyMS/controller/AttitudeController.h"
#include "flyMS/controller/DigitalFilter.h"
#include "flyMS/controller/Setpoint.h"
#include "flyMS/hardware/Imu.h"
#include "flyMS/hardware/gps.h"
#include "flyMS/hardware/pru/PruRequester.h"
#include "flyMS/ipc/mavlink_redis/MavlinkRedisPub.h"
#include "flyMS/ipc/mavlink_redis/MavlinkRedisSubQueue.h"
#include "flyMS/ulog/ulog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"

namespace flyMS {

static constexpr char kFLY_STEREO_CHANNEL[] = "mission_mavlink_data";

/**
 * @brief Object responsible for the inner loop of the flight controller.
 *
 */
class FlightCore {
 public:
  /**
   * @brief Construct a new Flight Core object
   *
   * @param input_params Input parameters needed for flight
   */
  FlightCore(const YAML::Node &input_params, std::shared_ptr<PruRequester> pru_requester);

  FlightCore(Setpoint &&setpoint, PositionController &&position_controller, const YAML::Node &config_params,
             std::shared_ptr<PruRequester> pru_requester);

  // TODO: add a non-yaml constructor
  ~FlightCore() = default;
  FlightCore() = delete;
  FlightCore(const FlightCore &) = delete;
  FlightCore(FlightCore &&) = delete;
  FlightCore &operator=(const FlightCore &) = delete;
  FlightCore &operator=(FlightCore &&) = delete;

  /**
   * @brief Perform all initialization tasks and start threads
   *
   * @return true if initialization was successful
   * @return false if initialization failed
   */
  bool init();

 private:
  /**
   * @brief Main control loop calculation for the flight stack. This function is registered to the Imu object as a
   * callback which gets invoked every time the IMU signals data is available to process
   *
   * @param imu_data_body
   */
  void flight_core(StateData &imu_data_body);

  /**
   * @brief Initializes all logging for flyMS. First, it creates a unique log directory 'runXXX' (datetime not used
   * because it is typically not synced). Then in initialized spdlog, then ulog
   *
   * @param log_dir location to store log directories
   */
  void init_logging(const std::filesystem::path &log_location);

  /**
   * @brief Debug function which writes system into to the console. Typically only called in debug mode
   *
   * @param imu_data_body imu data in body frame
   * @param setpoint setpoint data
   */
  void console_print(const StateData &imu_data_body, const std::vector<float> &setpoints, const std::vector<float> &u);

  /**
   * @brief Apply a saturation filter on motor command data. If any of the motor outputs are greater than 1, decrease
   * the set of commands evenly
   *
   * @param u The motor commands
   */
  void output_saturation_filter(std::vector<float> &u);

  // Configurable parameters
  FlightMode flight_mode_;                       // TODO remove this from flight core
  std::string log_filepath_;                     //< The filepath to the logging directory
  YAML::Node config_params_;                     //< A copy of the yaml configuration
  std::shared_ptr<PruRequester> pru_requester_;  //< Object to handle ownership and coms with PRU

  // Variables for working with flyStereo
  bool flyStereo_running_ = false;
  bool flyStereo_streaming_data_ = false;
  float standing_throttle_ = 0.0f;
  float initial_yaw_ = 0.0f;

  // Counter for number of timestamps at min throttle, used to detect landing
  // and reset the integrators in the PID controllers
  int integrator_reset_ = 0;

  Imu &imu_module_;  //< Reference to Imu singleton object and Data struct from the imu manager
  ULog ulog_;        //< Class to handle and write to the log file

  Setpoint setpoint_module_;                //< Object and Data struct from the setpoint manager
  AttitudeController attitude_controller_;  //< Converts desired setpoint data & state data to state control signals
  PositionController position_controller_;  //< Controller for position when flying using flyStereo
  DigitalFilter gyro_lpf_pitch_;            //< Low pass filter for imu pitch measurement
  DigitalFilter gyro_lpf_roll_;             //< Low pass filter for imu roll measurement
  DigitalFilter gyro_lpf_yaw_;              //< Low pass filter for imu yaw measurement

  std::unique_ptr<MavlinkRedisSubQueue> mavlink_subscriber_;      //< Receive mavlink messages from Redis
  std::unique_ptr<MavlinkRedisPub> mavlink_publisher_;            //< Publish mavlink messages from Redis
  std::shared_ptr<IpcQueue<mavlink_odometry_t>> odometry_queue_;  //< A shared queue to receive odometry data
};

}  // namespace flyMS
