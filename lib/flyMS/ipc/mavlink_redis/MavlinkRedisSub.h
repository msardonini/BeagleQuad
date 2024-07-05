/**
 * @file redis_interface.h
 * @author Mike Sardonini
 * @brief Header file for the MavlinkRedisSub, an object which assists in inter-process communication for using Redis
 * and Mavlink Messages
 *
 * @version 0.1
 * @date 2022-08-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include "flyMS/ipc/mavlink/MavlinkParser.h"
#include "flyMS/ipc/redis/RedisSubscriberQueue.h"
#include "mavlink_v2/common/mavlink.h"
#include "redis++.h"
#include "spdlog/spdlog.h"

namespace flyMS {

/**
 * @brief Subscribe to a Redis channel and parse the messages as Mavlink messages. The desired messages to parse must be
 registered with this object. 'Registering' requires the user to provide the mavlink decode
 function, and the callback function that will be invoked when messages are received.
 *
 * Example Usage:
 * @code
 * auto callback = [](mavlink_odometry_t& msg) { std::cout << "Received message: " << msg.x << std::endl; };
 * MavlinkRedisSub redis_sub("test_mavlink_channel");
 * redis_sub.register_message<mavlink_odometry_t, MAVLINK_MSG_ID_ODOMETRY>(callback, &mavlink_msg_odometry_decode);
 * while (true) {
 *   // do something and wait for messages
 * }
 * @endcode
 */
class MavlinkRedisSub : public RedisSubscriber, public MavlinkParser {
 public:
  /**
   * @brief Construct a new Redis Interface object
   *
   */
  MavlinkRedisSub(const std::string& channel);

  /**
   * @brief Destroy the Redis Interface object
   *
   */
  ~MavlinkRedisSub();

 private:
  /**
   * @brief Callback for the Redis subscriber. This callback parses bytes into generic mavlink messages and then passed
   * them to the MavlinkParser for further decoding
   *
   * @param channel The channel the message was received on
   * @param msg The contents of the message
   */
  void MavlinkMessageCallback(std::string channel, std::string msg);
};

}  // namespace flyMS
