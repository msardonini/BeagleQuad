/**
 * @file RedisSubscriber.h
 * @author Mike Sardonini
 * @brief Declaration of RedisSubscriber class
 *
 * @version 0.1
 * @date 2022-08-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "mavlink_v2/common/mavlink.h"
#include "redis++.h"
#include "spdlog/spdlog.h"

namespace flyMS {

/**
 * @brief RedisSubscriber class. This class is a simple wrapper around the redis++ subscriber, but it applies default
 * settings for flyMS. It receives messages from the RedisPublisher class.
 *
 * Example Usage:
 * @code {.cpp}
 * std::mutex mutex;
 * std::string my_received_message;
 * auto callback = [&](auto channel, auto message) {
 *   std::unique_lock<std::mutex> lock(mutex);
 *   my_received_message = message;
 * };
 * RedisSubscriber sub(callback, {"test_channel"});
 *
 * while (true) {
 *   // Lock mutex and do stuff with my_received_message
 * }
 * @endcode
 */
class RedisSubscriber {
 public:
  /**
   * @brief Construct a new Redis Interface object
   *
   * @param callback The callback function to invoke when a message is received on one of the subscribed channels
   * @param channels The channels to subscribe to
   * @param error_callback The callback function to invoke when an error occurs, this includes timeouts when messages
   * are not received within the expected period
   */
  RedisSubscriber(
      std::function<void(std::string, std::string)> callback, const std::vector<std::string> &channels,
      std::function<void(const sw::redis::Error &)> error_callback = RedisSubscriber::default_timeout_callback);

  /**
   * @brief Destroy the RedisSubscriber object
   *
   */
  virtual ~RedisSubscriber();

  /**
   * @brief Set the error callback function. This function will be called if the internal redis++ subscriber throws an
   * exeption
   *
   * @param callback The callback function to invoke when an error occurs
   */
  void set_error_callback(std::function<void(const sw::redis::Error)> callback);

 private:
  /**
   * @brief Thread for the Redis subscriber. Runs in a loop until is_running_ is false
   *
   */
  void subscribe_thread();

  /**
   * @brief Default behavior for action when the Redis subscriber throws an exception
   *
   * @param err The exception that was thrown
   */
  static void default_timeout_callback(const sw::redis::Error &err);

  std::function<void(const sw::redis::Error &err)>
      on_error_;  //< Function to be called when the Redis subscriber throws an error

  std::atomic<bool> is_running_{false};         //< Flag to indicate if the subscriber thread is running
  std::thread subscribe_thread_;                //< Thread for the Redis subscriber
  std::unique_ptr<sw::redis::Redis> redis_;     //< Redis Interface object
  std::unique_ptr<sw::redis::Subscriber> sub_;  //< Redis subscriber object
};

}  // namespace flyMS
