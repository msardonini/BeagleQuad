#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <thread>

namespace flyMS {

/**
 * @brief Send and receive data over a UART port. Baudrate is set to B115200.
 *
 * Example Usage:
 * @code
 * auto callback = [](const std::array<uint8_t, kBUF_SIZE>& bytes, size_t size) {
 *   // do something with the received bytes
 * };
 * Uart uart("/dev/ttyUSB0", callback);
 * uart.send({0x01, 0x02, 0x03});
 * @endcode
 */
class Uart {
 public:
  static constexpr std::size_t kBUF_SIZE = 1024;

  /**
   * @brief Construct a new Uart object
   *
   * @param serial_dev The path to the serial device
   * @param callback The callback function that is called when new data is received
   */
  Uart(const std::filesystem::path& serial_dev,
       std::function<void(const std::array<uint8_t, kBUF_SIZE>&, size_t)> callback);

  virtual ~Uart() = default;

  /**
   * @brief Send data over the UART port
   *
   * @param bytes The data to send
   * @return true Data was sent successfully
   * @return false There was an error sending data
   */
  bool send(const std::vector<uint8_t>& bytes);

 private:
  /**
   * @brief Thread which reads data from the UART port, and calls the callback function. This function
   * shuts down when the is_running_ atomic variable is set to false, which happens in the destructor.
   *
   */
  void serial_read_thread();

  std::atomic<bool> is_running_;  //< Flag to indicate if the serial_read_thread should be running
  std::function<void(const std::array<uint8_t, kBUF_SIZE>&, size_t)> callback_;  //< Callback function
  int serial_fd_;                                                                //< File descriptor for the serial port
  std::thread serial_read_thread_;  //< Thread that reads the serial port for new data
};

}  // namespace flyMS
