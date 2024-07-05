#pragma once

#include "flyMS/ipc/redis/RedisPublisher.h"
#include "mavlink_v2/common/mavlink.h"

namespace flyMS {

/**
 * @brief Publish Mavlink messages to a Redis channel. Any type of Mavlink message can be published.
 *
 * Example Usage:
 * @code
 * MavlinkRedisPub pub;
 * mavlink_odometry_t odom_msg;
 * // Fill contents of odom_msg..
 * pub.publish("test_redis_channel", odom_msg, &mavlink_msg_odometry_encode);
 * @endcode
 *
 */
class MavlinkRedisPub : private RedisPublisher {
 public:
  /**
   * @brief Publish a Mavlink message to a Redis channel
   *
   * @tparam MavType The type of the mavlink message to publish
   * @param channel The Redis channel to publish the message to
   * @param msg The message to publish
   * @param encode_func The encode funtion for the mavlink message type. This is supplied by the mavlink library. It
   * will have the form: 'void mavlink_msg_<message_name>_encode(uint8_t system_id, uint8_t component_id,
   * mavlink_message_t* msg, const mavlink_<message_name>_t* <message_name>)'
   */
  template <typename MavType>
  void publish(std::string_view channel, MavType& msg,
               std::function<uint16_t(uint8_t, uint8_t, mavlink_message_t*, const MavType*)> encode_func) {
    mavlink_message_t mav_msg;
    encode_func(system_id_, component_id_, &mav_msg, &msg);
    publish(channel, mav_msg);
  }

  /**
   * @brief Publish a generic mavlink_message_t to a Redis channel
   *
   * @param channel The Redis channel to publish the message to
   * @param msg The messsage to publish
   */
  void publish(std::string_view channel, const mavlink_message_t& msg) {
    std::array<uint8_t, 1024> buf;
    const auto len = mavlink_msg_to_send_buffer(buf.data(), &msg);
    std::string_view msg_str(reinterpret_cast<char*>(buf.data()), len);
    RedisPublisher::publish(channel, msg_str);
  }

  /**
   * @brief Set the System ID for the Mavlink messages. This is not required publish messages
   *
   * @param system_id The system ID to use
   */
  void set_system_id(uint8_t system_id) { system_id_ = system_id; }

  /**
   * @brief Set the Component ID for the Mavlink messages. This is not required publish messages
   *
   * @param component_id The component ID to use
   */
  void set_component_id(uint8_t component_id) { component_id_ = component_id; }

 private:
  uint8_t system_id_ = 0;
  uint8_t component_id_ = 0;
};

}  // namespace flyMS
