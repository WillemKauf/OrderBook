#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

/////////////////
/// std
/////////////////
#include <unistd.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

/////////////////
/// local
/////////////////
#include "order_book.h"
#include "serialization.h"
#include "socket_wrappers.h"

class ClientManager {
public:
  ClientManager(const ClientManager&)  = delete;
  void operator=(const ClientManager&) = delete;

  static ClientManager& GetClientManager() {
    static ClientManager orderManager;
    return orderManager;
  }

  // Blocks on destruction to flush queue.
  ~ClientManager() {
    FlushQueue();

    freeaddrinfo(clientAddrInfo_);
    close(clientFd_);
  }

  void SubmitOrder(const Order& order) {
    std::scoped_lock lock(mutex_);
    orderQueue_.push(order);
    cv_.notify_one();
  }

private:
  void FlushQueue() {
    std::unique_lock uniqueLock(mutex_);
    cv_.wait(uniqueLock, [&] { return orderQueue_.empty(); });
    stopFlag_ = true;
    cv_.notify_all();
  }

  void Run() {
    while (true) {
      std::unique_lock uniqueLock(mutex_);
      cv_.wait(uniqueLock, [&] { return !orderQueue_.empty() || stopFlag_; });
      if (stopFlag_) {
        break;
      }
      auto order = orderQueue_.front();
      orderQueue_.pop();
      SendOrder(std::move(order));
      // In case we need to flush the queue.
      if (orderQueue_.empty()) {
        cv_.notify_all();
      }
    }
  }

  void SendOrder(Order order) {
    static socklen_t clientAddrInfoLen = sizeof(addrinfo);
    SerializeAndSend(std::move(order), clientFd_, clientAddrInfo_->ai_addr, clientAddrInfoLen);
  }

  ClientManager() {
    SetAddrInfo(&clientAddrInfo_);
    SetSocket(clientAddrInfo_, clientFd_);
    thread_ = std::jthread(&ClientManager::Run, this);
  }

  std::jthread thread_;
  std::mutex mutex_;
  std::queue<Order> orderQueue_;
  std::atomic<bool> stopFlag_ = false;
  std::condition_variable cv_;
  addrinfo* clientAddrInfo_ = nullptr;
  int clientFd_             = -1;
};

#endif  // #ifndef CLIENT_MANAGER_H
