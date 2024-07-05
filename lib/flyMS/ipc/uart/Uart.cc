#include "Uart.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "spdlog/spdlog.h"

namespace flyMS {

Uart::Uart(const std::filesystem::path &serial_dev,
           std::function<void(const std::array<uint8_t, kBUF_SIZE> &, size_t)> callback)
    : callback_(callback) {
  // Open the serial port
  serial_fd_ = open(serial_dev.string().c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (serial_fd_ == -1) {
    throw std::runtime_error("Failed to open the serial device");
    return;
  }

  // Ensure the port is set to NONBLOCK
  fcntl(serial_fd_, F_SETFL, fcntl(serial_fd_, F_GETFL) & ~O_NONBLOCK);

  // Get the current configuration of the serial interface
  struct termios serial_config;
  if (tcgetattr(serial_fd_, &serial_config) < 0) {
    throw std::runtime_error("Failed to get the serial device attributes");
    return;
  }

  // Set the serial device configs
  serial_config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
  serial_config.c_oflag = 0;
  serial_config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  serial_config.c_cflag &= ~(CSIZE | PARENB);
  serial_config.c_cflag |= CS8;

  // One input byte is enough to return from read() Inter-character timer off
  serial_config.c_cc[VMIN] = 0;
  serial_config.c_cc[VTIME] = 10;

  // Communication speed (simple version, using the predefined constants)
  if (cfsetispeed(&serial_config, B115200) < 0 || cfsetospeed(&serial_config, B115200) < 0) {
    throw std::runtime_error("Failed to set the baudrate on the serial port!");
    return;
  }

  // Finally, apply the configuration
  if (tcsetattr(serial_fd_, TCSAFLUSH, &serial_config) < 0) {
    throw std::runtime_error("Failed to set the baudrate on the serial port!");
    return;
  }
  serial_read_thread_ = std::thread(&Uart::serial_read_thread, this);

  is_running_ = true;
  return;
}

bool Uart::send(const std::vector<uint8_t> &bytes) {
  if (serial_fd_ == -1) {
    spdlog::error("Failed to send data to the serial port, the port is not open!");
    return false;
  }

  ssize_t bytes_written = write(serial_fd_, bytes.data(), bytes.size());
  if (bytes_written < 0) {
    spdlog::error("Failed to write to the serial port!");
    return false;
  } else if (static_cast<std::size_t>(bytes_written) != bytes.size()) {
    spdlog::error("Failed to send data to the serial port!");
    return false;
  } else {
    return true;
  }
}

void Uart::serial_read_thread() {
  constexpr std::size_t buf_size = 1024;
  std::array<unsigned char, buf_size> buf;
  while (is_running_) {
    ssize_t ret = read(serial_fd_, buf.data(), buf_size);
    if (ret < 0) {
      spdlog::error("Error on read(), errno: {}", strerror(errno));
    } else if (ret > 0) {
      callback_(buf, ret);
    } else {
      // Sleep so we don't overload the CPU. This isn't an ideal method, but if use a blocking call on read(), we
      // can't break out of it on the destruction of this object. It will hang forever until bytes are read, which is
      // not always the case
      // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

}  // namespace flyMS
