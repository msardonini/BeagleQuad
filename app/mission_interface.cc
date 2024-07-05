/**
 * @file flyStereo_interface.cc
 * @author Mike Sardonini (msardonini@gmail.com)
 * @brief This file contains the implementation of the interface between flyMS and flyStereo. It owns the serial port
 * that connects to the mission computer, and communicates with the flyMS app using redis. All messages are in mavlink
 * format, and mavlink is used to serialize, deserialize, messages
 * @version 0.1
 * @date 2022-07-31
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "flyMS/ipc/mavlink/MavlinkUart.h"
#include "flyMS/ipc/mavlink_redis/MavlinkRedisPub.h"
#include "flyMS/ipc/mavlink_redis/MavlinkRedisSub.h"

static constexpr char kMAVLINK_FLIGHT_CORE_CHANNEL[] = "mission_mavlink_data";
static constexpr char kMAVLINK_WEBSERVER_CHANNEL[] = "webserver_mavlink_data";
static constexpr char kSERIAL_PORT[] = "/dev/ttyS5";
std::atomic<bool> is_running_{true};

/**
 * @brief Signal handler. Ignore SIGHUP, others will shut down the process
 *
 * @param signo
 */
static void on_signal_received(int signo) {
  switch (signo) {
    case SIGHUP:
      break;
    default:
      is_running_ = false;
  }
}

int main() {
  // Register System Signals to be handled by on_signal_received
  signal(SIGINT, on_signal_received);
  signal(SIGKILL, on_signal_received);
  signal(SIGHUP, on_signal_received);

  flyMS::MavlinkUart mav_uart(kSERIAL_PORT);
  flyMS::MavlinkRedisPub mav_pub;
  flyMS::MavlinkRedisSub mav_sub(kMAVLINK_FLIGHT_CORE_CHANNEL);

  // Heartbeat messages from the Mission Computer get sent to the Redis channel
  auto heartbeat_msg_callback = [&mav_pub](auto& msg) {
    mav_pub.publish<mavlink_heartbeat_t>(kMAVLINK_FLIGHT_CORE_CHANNEL, msg, &mavlink_msg_heartbeat_encode);
  };
  mav_uart.register_message<mavlink_heartbeat_t, MAVLINK_MSG_ID_HEARTBEAT>(heartbeat_msg_callback,
                                                                           &mavlink_msg_heartbeat_decode);

  // Odometry messages from the Mission Computer get sent to the Redis channel
  auto odom_msg_callback = [&mav_pub](auto& msg) {
    mav_pub.publish<mavlink_odometry_t>(kMAVLINK_FLIGHT_CORE_CHANNEL, msg, &mavlink_msg_odometry_encode);
  };
  mav_uart.register_message<mavlink_odometry_t, MAVLINK_MSG_ID_ODOMETRY>(odom_msg_callback,
                                                                         &mavlink_msg_odometry_decode);

  // Command messages are sent from the webserver to the Mission Computer
  auto command_msg_callback = [&mav_uart](auto& msg) {
    mav_uart.send_mavlink_msg<mavlink_command_long_t>(msg, &mavlink_msg_command_long_encode);
  };
  mav_sub.register_message<mavlink_command_long_t, MAVLINK_MSG_ID_COMMAND_LONG>(command_msg_callback,
                                                                                &mavlink_msg_command_long_decode);

  // Command messages are sent from flyMS to the Mission Computer
  auto command_msg_callback_int = [&mav_uart](auto& msg) {
    std::cout << "Received command_int message" << std::endl;
    mav_uart.send_mavlink_msg<mavlink_command_int_t>(msg, &mavlink_msg_command_int_encode);
  };
  mav_sub.register_message<mavlink_command_int_t, MAVLINK_MSG_ID_COMMAND_INT>(command_msg_callback_int,
                                                                              &mavlink_msg_command_int_decode);

  while (is_running_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
