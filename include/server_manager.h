#ifndef SERVER_H
#define SERVER_H

/////////////////
/// std
/////////////////
#include <unistd.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <fstream>

/////////////////
/// local
/////////////////
#include "config.h"
#include "order_book.h"
#include "serialization.h"
#include "socket_wrappers.h"

class ServerManager {
public:
  ServerManager(const ServerManager&)  = delete;
  void operator=(const ServerManager&) = delete;

  static ServerManager& GetServerManager() {
    static ServerManager serverManager;
    return serverManager;
  }

  ~ServerManager() {
    runThread_.join();
    publishThread_.join();

    freeaddrinfo(serverAddrInfo_);
    close(serverFd_);
  }

private:
  void Run() {
    static socklen_t addrStorageLen = sizeof(addrStorage_);
    while (!stopFlag_) {
      Order order;
      auto status = RecvAndDeserialize(order, serverFd_, reinterpret_cast<sockaddr*>(&addrStorage_),
                                       &addrStorageLen);
      if (status == 0) {
        // Timeout, assume program is over.
        std::cout << "No orders recieved, server shutting down.\n";
        stopFlag_ = true;
        cv_.notify_all();
        break;
      }
      const auto logVec = orderBooks_.HandleOrder(std::move(order));
      if(!logVec.has_value()){ //Flush orderbooks.
        FlushBooks();
      }else{
        AddToServerLog(logVec.value());
      }
    }
  }

  void FlushBooks(){
    cv_.notify_one(); // Notify publish thread.
    std::unique_lock uniqueLock(mutex_); //Get a unique lock.
    cv_.wait(uniqueLock, [&] { return serverLog_.empty(); }); //Block until serverLog is emptied.
  }

  void AddToServerLog(const std::vector<std::string>& logVec){
    std::unique_lock uniqueLock(mutex_); //Get a unique lock.
    for (const auto& logStr : logVec) {
      serverLog_.push(logStr);
    }
  }

  void WriteLog(){
    std::cout << "============Writing Log " << ++id_ << "============\n";
    const std::string outputFileStr = ROOT_DIR + "/logs/" + std::to_string(id_) + ".log";
    std::ofstream outputFile(outputFileStr);
    if(!outputFile.is_open()){
      throw std::runtime_error("Couldn't write to outputFile in WriteLog");
    }
    while(!serverLog_.empty()){
      auto logMsg = serverLog_.front();
      serverLog_.pop();
      outputFile << logMsg << '\n';
      std::cout << logMsg << '\n';
    }
    outputFile.close();
  }

  void Publish(){
    while (!stopFlag_) {
      std::unique_lock uniqueLock(mutex_);
      cv_.wait(uniqueLock, [&] { return !serverLog_.empty() || stopFlag_; });
      if (stopFlag_) {
        break;
      }
      WriteLog();
      cv_.notify_all();
    }
  }

  ServerManager() {
    SetAddrInfo(&serverAddrInfo_);
    SetSocket(serverAddrInfo_, serverFd_);
    BindSocket(serverAddrInfo_, serverFd_);
    runThread_ = std::jthread(&ServerManager::Run, this);
    publishThread_ = std::jthread(&ServerManager::Publish, this);
  }

  int serverFd_             = -1;
  std::jthread runThread_;
  std::jthread publishThread_;
  addrinfo* serverAddrInfo_ = nullptr;
  sockaddr_storage addrStorage_{};
  OrderBooks orderBooks_;
  std::atomic<bool> stopFlag_ = false;
  std::condition_variable cv_;
  std::mutex mutex_;
  std::queue<std::string> serverLog_;
  std::atomic<int> id_ = 0;
};

#endif  // #ifndef SERVER_H
