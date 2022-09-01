/**
 * @file redis_interface.cc
 * @author Mike Sardonini (msardonini@gmail.com)
 * @brief Source file for the RedisInterface, an object which assists in inter-process communication for using Redis and
 * Mavlink Messages
 * @version 0.1
 * @date 2022-08-12
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "flyMS/RedisInterface.h"

namespace flyMS {

RedisInterface::RedisInterface(std::size_t max_queue_size) : max_queue_size_(max_queue_size) {
  sw::redis::ConnectionOptions opts1;
  opts1.host = kRedisHost;
  opts1.port = kRedisPort;
  opts1.socket_timeout = kRedisTimeout;

  redis_ = std::make_unique<sw::redis::Redis>(opts1);
  sub_ = std::make_unique<sw::redis::Subscriber>(redis_->subscriber());
  sub_->on_message(
      std::bind(&RedisInterface::MavlinkMessageCallback, this, std::placeholders::_1, std::placeholders::_2));

  is_running_ = true;
  subscribe_thread_ = std::thread(&RedisInterface::subscribe_thread, this);
}

RedisInterface::~RedisInterface() {
  is_running_ = false;
  if (subscribe_thread_.joinable()) {
    subscribe_thread_.join();
  }
}

void RedisInterface::subscribe_to_channel(std::string_view channel) { sub_->subscribe(channel); }

std::tuple<VioData, bool> RedisInterface::GetVioData() {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  if (queue_vio_data_.empty()) {
    return std::make_tuple(VioData(), false);
  } else {
    auto data = queue_vio_data_.front();
    queue_vio_data_.pop();
    return std::make_tuple(data, true);
  }
}

VioData RedisInterface::ConvertFromMavlink(mavlink_odometry_t &msg) {
  VioData vio_data;
  vio_data.position << msg.x, msg.y, msg.z;
  vio_data.velocity << msg.vx, msg.vy, msg.vz;
  vio_data.quat = Eigen::Quaternion(msg.q[0], msg.q[1], msg.q[2], msg.q[3]);
  return vio_data;
}

void RedisInterface::MavlinkMessageCallback(std::string channel, std::string msg) {
  for (auto mav_char : msg) {
    mavlink_message_t mav_msg;
    mavlink_status_t mav_status;
    auto found = mavlink_parse_char(MAVLINK_COMM_2, mav_char, &mav_msg, &mav_status);
    if (found) {
      switch (mav_msg.msgid) {
        case MAVLINK_MSG_ID_ODOMETRY: {
          mavlink_odometry_t odometry_msg;
          mavlink_msg_odometry_decode(&mav_msg, &odometry_msg);
          auto vio_data = ConvertFromMavlink(odometry_msg);
          std::lock_guard<std::mutex> lock(queue_mutex_);
          queue_vio_data_.push(vio_data);
          if (queue_vio_data_.size() > max_queue_size_) {
            queue_vio_data_.pop();
          }
          break;
        }
      }
    }
  }
}

void RedisInterface::subscribe_thread() {
  while (is_running_.load()) {
    try {
      sub_->consume();
    } catch (const sw::redis::Error &err) {
      spdlog::warn("Timeout exceeded, err: {}", err.what());
    }
  }
}

}  // namespace flyMS