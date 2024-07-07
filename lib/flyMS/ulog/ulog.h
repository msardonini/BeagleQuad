/****************************************************************************
 *
 *   Copyright (c) 2016 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "flyMS/common/state_data.h"
#include "flyMS/ulog/ulog_messages.h"
#include "spdlog/fmt/fmt.h"

namespace flyMS {

/**
 * @brief Class to handle the logging of data to a file in the ULog data format
 * (https://docs.px4.io/main/en/dev_log/ulog_file_format.html). The interface is relatively simple. The user
 * instantiates the object, runs the init() member function once, and then repeatedly calls the `write_msg()`
 * member function to record the data to a file. The user can then use the `px4tools` python package to read the data
 * from the file (typically it is converted to a CSV file). To increase the performance of the logging, the interaction
 * with the file system is done in a separate thread. This way, the control thread doesn't get backed up with I/O
 * flushing
 *
 */
class ULog {
 public:
  /**
   * @brief Construct a new ULog object
   *
   */
  ULog() = default;

  /**
   * @brief Destroy the ULog object
   *
   */
  ~ULog();

  /**
   * @brief Initialize the ULog object. This function must be called before any other member functions. It creates the
   * file, writes the header, and starts the logging thread. The created file is called logger.ulog, and it placed in
   * the directory provided
   *
   * @param log_filename The name of the ULog file to create
   * @return int 0 if successful, -1 otherwise
   */
  int init(const std::filesystem::path &log_folder);

  /**
   * @brief Writes a generic ULogMsg to the log file. It serializes the data, then adds it to a queue to be written to
   * the file. The queue is then flushed to the file in the logging thread
   *
   * @param data The ULogMsg to write to the file
   */
  void write_msg(const ULogMsg &data);

  /**
   * @brief Creates a directory for logging data for a given run. This is run with a very simple naming scheme, where
   * the directory is named `run<number>`, where `<number>` is the first number that doesn't already exist. The date or
   * time is not used because the hardware usually doesn't have a clock that is synchronized with local time
   *
   * @param log_folder The base folder to create the run directory in
   * @return std::filesystem::path The path to the created directory
   */
  static std::filesystem::path generate_incremented_run_dir(const std::filesystem::path &log_folder);

 private:
  /**
   * @brief Get the time in microseconds since the epoch
   *
   * @return uint64_t
   */
  uint64_t get_time_us();

  /**
   * @brief Writes the generic ULog header to the file
   *
   */
  void write_header();

  /**
   * @brief Writes the ULog msg format to the header portion of the file
   *
   * @param msg_format The message format to write in the header
   */
  void write_formats(std::string_view msg_format);

  /**
   * @brief Registers the Ulog message formats with a name and ID
   *
   * @param id The ID of the message
   * @param msg_name The name of the message
   */
  void write_add_log(uint16_t id, std::string_view msg_name);

  /**
   * @brief Write bytes to the file
   *
   * @param buf Pointer to the data to write
   * @param size The number of bytes to write
   */
  void write_to_file(const void *buf, size_t size);

  /**
   * @brief The thread that handles the flushing of the data to the file
   *
   */
  void write_thread();

  std::atomic<bool> is_running_ = false;                                           //< thread running flag
  std::thread write_thread_;                                                       //< thread for writing data
  std::mutex queue_mutex_;                                                         //< mutex for accessing the queue
  std::queue<std::pair<std::unique_ptr<const char[]>, std::size_t>> write_queue_;  //< queue for writing data
  std::ofstream fd_;  //< File descriptor for interfacing with the file
  std::map<uint16_t, std::pair<const char *, const char *>>
      msg_formats_;  //< Map of message formats for supported messages. Of the value pair, the first is the name of the
                     // message, and the second is the format of the message
};

}  // namespace flyMS
