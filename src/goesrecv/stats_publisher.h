#pragma once

#include "publisher.h"
#include "types.h"

class StatsPublisher : public Publisher {
public:
  static std::unique_ptr<StatsPublisher> create(const std::string& endpoint);

  explicit StatsPublisher(int fd);
  virtual ~StatsPublisher();

  void publish(const std::string& str);
};
