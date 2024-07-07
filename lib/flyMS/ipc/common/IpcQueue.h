#pragma once

#include <mutex>
#include <queue>

#include "spdlog/spdlog.h"

namespace flyMS {

/**
 * @brief A thread-safe queue to use as an intermediate receiving container
 *
 * @tparam MsgType The type of the message to be held in the queue
 */
template <typename MsgType>
struct IpcQueue {
 public:
  /**
   * @brief Construct a new Ipc Queue object
   *
   * @param max_queue_size The maximum number of elements to store in the queue. The default is 0, which means no
   * limit
   */
  IpcQueue(std::size_t max_queue_size = 0) : max_queue_size_(max_queue_size) {}

  /**
   * @brief Gets the front of the queue
   *
   * @return MsgType
   */
  MsgType front() {
    std::scoped_lock<std::mutex> lock(mutex_);
    return queue_.front();
  }

  /**
   * @brief Deletes the front-most point of the queue
   *
   */
  void pop() {
    std::scoped_lock<std::mutex> lock(mutex_);
    queue_.pop();
  }

  /**
   * @brief Moves the front most element out of the queue, and returns it. Also the element is deleted from the queue
   *
   * @return MsgType
   */
  MsgType pop_front() {
    std::scoped_lock<std::mutex> lock(mutex_);
    auto front = std::move(queue_.front());
    queue_.pop();
    return front;
  }

  /**
   * @brief Push an element to the end of the queue
   *
   * @param msg
   */
  void push(MsgType& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (max_queue_size_ != 0 && queue_.size() >= max_queue_size_) {
      spdlog::warn("IpcQueue is full, dropping oldest message");
      queue_.pop();
    }

    queue_.push(msg);
  }

  /**
   * @brief Get the size of the queue
   *
   * @return std::size_t
   */
  std::size_t size() const { return queue_.size(); }

  /**
   * @brief Check if the queue is empty
   *
   * @return true The queue is empty
   * @return false The queue contains elements
   */
  bool empty() const { return queue_.empty(); }

 private:
  std::size_t max_queue_size_;
  std::mutex mutex_;
  std::queue<MsgType> queue_;
};
}  // namespace flyMS