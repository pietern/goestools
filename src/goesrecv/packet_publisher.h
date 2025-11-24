#pragma once

#include "publisher.h"
#include <cstdint>

class PacketPublisher : public Publisher {
public:
  static std::unique_ptr<PacketPublisher> create(const std::string& endpoint);

  explicit PacketPublisher(int fd);
  virtual ~PacketPublisher();

  void publish(const std::array<uint8_t, 892>& packet);
};
