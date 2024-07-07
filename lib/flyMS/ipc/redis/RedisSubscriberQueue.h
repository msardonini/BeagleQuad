
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "flyMS/ipc/common/IpcQueue.h"
#include "flyMS/ipc/redis/RedisSubscriber.h"
#include "spdlog/spdlog.h"

namespace flyMS {

class RedisSubscriberQueue {
 public:
  /**
   * @brief Construct a new RedisSubscriberQueue object
   *
   */
  RedisSubscriberQueue() = default;

  /**
   * @brief Destroy the RedisSubscriberQueue object
   *
   */
  ~RedisSubscriberQueue() = default;

  /**
   * @brief Registers a channel to listen to, and creates a queue that will receive messages from that queue
   *
   * @param channel The channel to listen to
   * @return std::shared_ptr<IpcQueue<std::string>>
   */
  std::shared_ptr<IpcQueue<std::string>> register_message(const std::string& channel) {
    if (redis_subscriber_) {
      spdlog::error("Tried to register message after RedisSubscriberQueue::start() was called");
      return nullptr;
    }

    auto redis_queue = std::make_shared<IpcQueue<std::string>>();
    redis_queues_.emplace(std::make_pair(channel, redis_queue));
    channels_.push_back(channel);
    return redis_queue;
  }

  /**
   * @brief Starts the RedisSubscriberQueue. All messages should be registered before this member function is called
   *
   */
  void start() {
    if (channels_.empty()) {
      spdlog::error(
          "Tried to start RedisSubscriberQueue with no messages registered! Call "
          "RedisSubscriberQueue::register_message() first");
    }

    auto callback_func = std::bind(&RedisSubscriberQueue::callback, this, std::placeholders::_1, std::placeholders::_2);
    redis_subscriber_ = std::make_unique<RedisSubscriber>(callback_func, channels_);
  }

 private:
  /**
   * @brief Simple callback function to provide to the RedisSubscriber. It will push the message to the appropriate
   * queue
   *
   * @param channel The channel the message was received on
   * @param msg The message content
   */
  void callback(std::string channel, std::string msg) {
    if (redis_queues_.find(channel) != redis_queues_.end()) {
      redis_queues_.find(channel)->second->push(msg);
    } else {
      spdlog::error("RedisSubscriberQueue: No queue registered for channel {}", channel);
    }
  }

  std::vector<std::string> channels_;                  //< The channels to listen to
  std::unique_ptr<RedisSubscriber> redis_subscriber_;  //< The subscriber which will receive the messages
  std::unordered_map<std::string, std::shared_ptr<IpcQueue<std::string>>>
      redis_queues_;  //< The queues to push the received messages to
};

}  // namespace flyMS
