#pragma once

#include <memory>
#include <unordered_map>

#include "mavlink_v2/common/mavlink.h"
#include "spdlog/spdlog.h"

namespace flyMS {

/**
 * @brief Base class to the templated MavlinkDecoderHelper class which provides a common interface for the decode
 * function.
 *
 */
struct MavlinkMsgDecodeHelperBase {
  virtual ~MavlinkMsgDecodeHelperBase() = default;

  /**
   * @brief Interface for the decode function. This function is implemented in the templated class, and sends its
   * user-defined output to the provided callback function
   *
   * @param msg The message to decode
   */
  virtual void decode(const mavlink_message_t& msg) = 0;

  /**
   * @brief Gets the message ID of the custom mavlink message type
   *
   * @return uint32_t
   */
  virtual uint32_t GetMsgId() const = 0;
};

/**
 * @brief Class to assist with the decoding of mavlink messages. That is, the de-serialization from `mavlink_message_t`
 * to the custom mavlink message type.
 *
 * @tparam MavType The custom mavlink message type
 * @tparam MsgId The ID of the custom mavlink message type
 */
template <typename MavType, uint32_t MsgId>
struct MavlinkMsgDecodeHelper : MavlinkMsgDecodeHelperBase {
  /**
   * @brief Construct a new Mavlink Msg Decode Helper object
   *
   * @param callback The callback function to send the decoded message to
   * @param decode The function to decode the message. This function is provided by mavlink. It typically has the
   * format: "mavlink_msg_<custom_msg_type>_decode"
   */
  MavlinkMsgDecodeHelper(std::function<void(MavType&)>&& callback,
                         std::function<void(const mavlink_message_t*, MavType*)>&& decode_func)
      : callback_(std::move(callback)), decode_func_(decode_func) {}

  /**
   * @brief Construct a new Mavlink Msg Decode Helper object
   *
   * @param callback The callback function to send the decoded message to
   * @param decode_func The function to decode the message. This function is provided by mavlink. It typically
   * has the format: "mavlink_msg_<custom_msg_type>_decode"
   */
  MavlinkMsgDecodeHelper(const std::function<void(MavType&)>& callback,
                         const std::function<void(const mavlink_message_t*, MavType*)>& decode_func)
      : callback_(callback), decode_func_(decode_func) {}

  /**
   * @brief Gets the message ID of the custom mavlink message type
   *
   * @return uint32_t
   */
  uint32_t GetMsgId() const override { return MsgId; }
  using ContainerType = MavType;

  /**
   * @brief Decodes a message and sends it to the callback function
   *
   * @param msg The message to decode
   */
  void decode(const mavlink_message_t& msg) override {
    MavType typed_msg;
    decode_func_(&msg, &typed_msg);
    callback_(typed_msg);
  }

  std::function<void(MavType&)> callback_;  //< The callback function to send the decoded message to
  std::function<void(const mavlink_message_t*, MavType*)> decode_func_;  //< The decode function
};

/**
 * @brief Decode a set of mavlink messages. That is, the de-serialization from `mavlink_message_t`
 * to the custom mavlink message type. Different message types are registered with this class during initialization,
 * and it will decode them and send them to the appropriate callback function. Serialized messages that need parsing
 * should be sent to the MavlinkParser::receive_mavlink_msg function.
 *
 * Example usage:
 * @code
 * MavlinkParser parser;
 * parser.register_message<mavlink_odometry_t, MAVLINK_MSG_ID_ODOMETRY>(
 *   [](mavlink_odometry_t& typed_msg) {
 *     std::cout << "Received odometry message with timestamp: " << typed_msg.time_usec << std::endl;
 *   },
 *   mavlink_msg_odometry_decode);
 *
 *   // .. Receive a mavlink_message_t via UART, REDIS, etc.
 *   parser.receive_mavlink_msg(msg);
 *   // .. The callback function will be called with the decoded message
 * @endcode
 */
class MavlinkParser {
 public:
  virtual ~MavlinkParser() = default;

  /**
   * @brief Registers a custom message type with this parser. A message cannot be parsed until it is registered. When a
   * message gets parsed, the callback function will be called with the decoded message.
   *
   * @tparam MavType The custom mavlink message type
   * @tparam MsgId The ID of the custom mavlink message type
   * @param callback The callback function to send the decoded message to
   * @param decode_func The function to decode the message. This function is provided by mavlink. It typically has the
   * format: "mavlink_msg_<custom_msg_type>_decode"
   */
  template <typename MavType, uint32_t MsgId>
  void register_message(std::function<void(MavType&)> callback,
                        std::function<void(const mavlink_message_t*, MavType*)> decode_func) {
    auto helper = std::make_unique<MavlinkMsgDecodeHelper<MavType, MsgId>>(callback, decode_func);

    mavlink_msg_decoders_.emplace(MsgId, std::move(helper));
  }

  /**
   * @brief Receives a serialized message and parses it. The message will be decoded and sent to the appropriate
   * callback
   *
   * @param msg The message to parse
   * @return true If the message was parsed successfully
   * @return false If the message was not parsed successfully
   */
  bool receive_mavlink_msg(const mavlink_message_t& msg) {
    auto it = mavlink_msg_decoders_.find(msg.msgid);
    if (it != mavlink_msg_decoders_.end()) {
      it->second->decode(msg);
      return true;
    } else {
      spdlog::warn(
          "No decoder found for message id: {}. Please register a decoder with MavlinkParser::register_message",
          static_cast<uint32_t>(msg.msgid));
      return false;
    }
  }

 private:
  std::unordered_map<uint32_t, std::unique_ptr<MavlinkMsgDecodeHelperBase>> mavlink_msg_decoders_;
};

}  // namespace flyMS
