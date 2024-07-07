#pragma once

#include <atomic>
#include <filesystem>
#include <thread>

#include "flyMS/ipc/common/IpcQueue.h"
#include "flyMS/ipc/mavlink/MavlinkParser.h"
#include "flyMS/ipc/uart/Uart.h"

namespace flyMS {

/**
 * @brief
 *
 */
class MavlinkUart : public MavlinkParser, private Uart {
 public:
  /**
   * @brief Construct a new Redis Interface object
   */
  MavlinkUart(const std::filesystem::path& serial_dev)
      : Uart(serial_dev, std::bind(&MavlinkUart::receive_bytes, this, std::placeholders::_1, std::placeholders::_2)) {}

  template <typename MavType>
  void send_mavlink_msg(MavType& msg,
                        std::function<uint16_t(uint8_t, uint8_t, mavlink_message_t*, const MavType*)> encode_func) {
    mavlink_message_t mav_message;
    auto size = encode_func(0, 0, &mav_message, &msg);
    std::vector<uint8_t> bytes(size);
    mavlink_msg_to_send_buffer(bytes.data(), &mav_message);
    send(bytes);
  }

  /**
   * @brief Register a message to be parsed by incoming mavlink messages via UART, and have the messages stored in a
   * thread safe queue when they are received
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

 private:
  void receive_bytes(const std::array<uint8_t, kBUF_SIZE>& bytes, size_t size);

  std::atomic<bool> is_running_;
  int serial_fd_;                   //< file descriptor for the serial port
  std::thread serial_read_thread_;  //< Thread that reads the serial port for new data

  bool init(const std::filesystem::path& serial_dev);

  void serial_read_thread();
};

}  // namespace flyMS
