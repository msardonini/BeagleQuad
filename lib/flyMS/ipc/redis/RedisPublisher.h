#pragma once
/**
 * @file RedisPublisher.h
 * @author Mike Sardonini
 * @brief RedisPublisher class
 * @version 0.1
 * @date 2022-09-21
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <sstream>

#include "flyMS/ipc/redis/redis_default_connection_opts.h"
#include "redis++.h"
#include "yaml-cpp/yaml.h"

namespace flyMS {

/**
 * @brief RedisPublisher class. This class is a simple wrapper around the redis++ publisher, but it applies default
 * settings for flyMS. It can be subscribed to using the RedisSubscriber class.
 *
 *  Exampe Usage:
 * @code {.cpp}
 * RedisPublisher pub;
 * std::string channel = "test_channel";
 * std::string message = "test_message";
 * pub.publish(channel, message);
 * @endcode
 *
 */
class RedisPublisher {
 public:
  /**
   * @brief Construct a new Redis Publisher object
   *
   */
  RedisPublisher() : redis_(std::make_unique<sw::redis::Redis>(redis_default_connection_opts())) {}

  virtual ~RedisPublisher() = default;

  /**
   * @brief Publish a message to a redis channel
   *
   * @param channel The channel to publish to
   * @param msg The contents of the message
   */
  void publish(std::string_view channel, std::string_view msg) { redis_->publish(channel, msg); }

  /**
   * @brief Publish a YAML node to the redis channel. The yaml node is converted to a string using it's << operator
   *
   * @param channel The channel to publish to
   * @param msg The yaml node to stringify then publish
   */
  void publish(std::string_view channel, const YAML::Node& msg) {
    std::stringstream msg_str;
    msg_str << msg;
    publish(channel, msg_str.str());
  }

 private:
  std::unique_ptr<sw::redis::Redis> redis_;
  std::vector<std::string> channels_;
};

}  // namespace flyMS
