#include "MavlinkUart.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace flyMS {

void MavlinkUart::receive_bytes(const std::array<uint8_t, kBUF_SIZE>& bytes, size_t size) {
  for (auto i = 0ul; i < size; i++) {
    mavlink_status_t mav_status;
    mavlink_message_t mav_message;
    // TODO: add mechanism to ensure only one object per process is using this channel
    uint8_t msg_received = mavlink_parse_char(MAVLINK_COMM_1, bytes[i], &mav_message, &mav_status);
    if (msg_received) {
      receive_mavlink_msg(mav_message);
    }
  }
}

}  // namespace flyMS
