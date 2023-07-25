#ifndef SOCKET_WRAPPERS_H
#define SOCKET_WRAPPERS_H

/////////////////
/// std
/////////////////
#include <errno.h>
#include <netdb.h>       //getaddrinfo
#include <sys/socket.h>  //sockets
#include <sys/types.h>   //types

/////////////////
/// local
/////////////////
#include "serialization.h"

#define PORT "8888"

void SetAddrInfo(addrinfo** addrInfo) {
  addrinfo tempAddrInfo{};
  tempAddrInfo.ai_family   = AF_INET6;
  tempAddrInfo.ai_socktype = SOCK_DGRAM;
  tempAddrInfo.ai_flags    = AI_PASSIVE;
  auto getAddrInfoStatus   = getaddrinfo(nullptr, PORT, &tempAddrInfo, addrInfo);
  if (getAddrInfoStatus < 0) {
    throw std::runtime_error("Error getting AddrInfo.");
  }
}

void SetSocket(addrinfo* addrInfo, int& fd, int secondsTimeOut = 2) {
  fd = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
  if (fd < 0) {
    throw std::runtime_error("Error setting socket.");
  }

  timeval timeVal{};
  timeVal.tv_sec = secondsTimeOut;
  auto setOptions = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeVal, sizeof(timeVal));
  if (setOptions < 0) {
    throw std::runtime_error("Error setting socket settings.");
  }
}

void BindSocket(addrinfo* addrInfo, int& fd) {
  auto bindStatus = bind(fd, addrInfo->ai_addr, addrInfo->ai_addrlen);
  if (bindStatus < 0) {
    throw std::runtime_error("Error binding socket.");
  }
}

template <typename T>
int SerializeAndSend(T&& object, int fd, const sockaddr* address, const socklen_t addressLen) {
  auto serializedObject = SerializeObject(std::forward<T>(object));

  auto sendStatus =
      sendto(fd, serializedObject.c_str(), serializedObject.size(), 0, address, addressLen);
  if(sendStatus != serializedObject.size()){
    throw std::runtime_error("Error sending in SerializeAndSend.");
  }
  return sendStatus;
}

template <typename T>
int RecvAndDeserialize(T& object, int fd, sockaddr* address, socklen_t* addressLen) {
  char buff[sizeof(T)] = {};
  auto readStatus      = recvfrom(fd, buff, sizeof(T), 0, address, addressLen);
  if (readStatus < 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      return 0;
    } else {
      throw std::runtime_error("Error recving in RecvAndDeserialize.");
    }
  }

  DeserializeObject(object, buff);
  return 1;
}

#endif  // #ifndef SOCKET_WRAPPERS_H
