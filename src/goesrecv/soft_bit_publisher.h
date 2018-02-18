#pragma once

#include "publisher.h"

class SoftBitPublisher : public Publisher {
public:
  static std::unique_ptr<SoftBitPublisher> create(const std::string& endpoint);

  explicit SoftBitPublisher(int fd);
  virtual ~SoftBitPublisher();

  void publish(const std::vector<int8_t>& bits);
};
