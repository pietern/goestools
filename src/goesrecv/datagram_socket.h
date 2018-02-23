#pragma once

#include <sys/socket.h>

#include <string>

class DatagramSocket {
public:
  explicit DatagramSocket(const std::string& addr);
  ~DatagramSocket();

  bool send(const std::string& payload);

protected:
  int fd_;
  sockaddr_storage addr_;
};
