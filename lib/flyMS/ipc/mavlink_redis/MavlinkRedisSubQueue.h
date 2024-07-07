#pragma once

#include "flyMS/ipc/mavlink_redis/MavlinkRedisSub.h"

namespace flyMS {

/**
 * @brief Subscribe to a Redis channel and parse the messages as Mavlink messages. This class is built on top of
 * MavlinkRedisSub, but adds the ability to queue messages instead of processing them via callback functions.
 *
 * Example Usage:
 * @code
 * MavlinkRedisSubQueue redis_sub("test_mavlink_channel");
 * auto queue = redis_sub.register_message_with_queue<mavlink_odometry_t,
 *    MAVLINK_MSG_ID_ODOMETRY>(&mavlink_msg_odometry_decode);
 *
 * while (true) {
 *   if (!queue->empty()) {
 *     auto msg = queue->pop_front();
 *     // process message
 *   }
 *   // sleep or do something else
 * }
 * @endcode
 */
class MavlinkRedisSubQueue : public MavlinkRedisSub {
 public:
  /**
   * @brief Construct a new MavlinkRedisSubQueue object
   *
   * @param channel The Redis channel to receive mavlink messages on
   */
  MavlinkRedisSubQueue(const std::string& channel) : MavlinkRedisSub(channel) {}

  /**
   * @brief Register a message to be parsed by incoming mavlink messages via the Redis channel, and receive a queue of
   * messages where the messages will be stored when they are received
   *
   * @tparam MavType The type of the mavlink message to parse
   * @tparam MsgId The id of the mavlink message to parse
   * @param decode The decode function for the mavlink message
   * @return std::shared_ptr<IpcQueue<MavType>>
   */
  template <typename MavType, uint32_t MsgId>
  std::shared_ptr<IpcQueue<MavType>> register_message_with_queue(
      std::function<void(const mavlink_message_t*, mavlink_odometry_t*)> decode) {
    auto queue = std::make_shared<IpcQueue<MavType>>();
    auto callback = [queue](auto& msg) { queue->push(msg); };
    register_message<MavType, MsgId>(callback, decode);
    return queue;
  }
};

}  // namespace flyMS
