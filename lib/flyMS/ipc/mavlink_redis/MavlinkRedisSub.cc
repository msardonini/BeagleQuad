/**
 * @file redis_interface.cc
 * @author Mike Sardonini (msardonini@gmail.com)
 * @brief Source file for the MavlinkRedisSub, an object which assists in inter-process communication for using Redis
 * and Mavlink Messages
 * @version 0.1
 * @date 2022-08-12
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "flyMS/ipc/mavlink_redis/MavlinkRedisSub.h"

namespace flyMS {

MavlinkRedisSub::MavlinkRedisSub(const std::string &channel)
    : RedisSubscriber(
          std::bind(&MavlinkRedisSub::MavlinkMessageCallback, this, std::placeholders::_1, std::placeholders::_2),
          {channel}) {}

MavlinkRedisSub::~MavlinkRedisSub() {}

void MavlinkRedisSub::MavlinkMessageCallback(std::string channel, std::string msg) {
  for (auto mav_char : msg) {
    mavlink_message_t mav_msg;
    mavlink_status_t mav_status;

    // TODO: make sure that no other object in this process is using the same mavlink channel
    // TODO: Error checking if message is not parsable
    auto found = mavlink_parse_char(MAVLINK_COMM_2, mav_char, &mav_msg, &mav_status);
    if (found) {
      receive_mavlink_msg(mav_msg);
    }
  }
}

}  // namespace flyMS
