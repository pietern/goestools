#pragma once

#include "publisher.h"
#include "types.h"

class SamplePublisher : public Publisher {
public:
  static std::unique_ptr<SamplePublisher> create(const std::string& endpoint);

  explicit SamplePublisher(int fd);
  virtual ~SamplePublisher();

  void publish(const Samples& samples);

protected:
  std::vector<std::complex<int8_t> > tmp_;
};
