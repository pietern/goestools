#pragma once

#include <complex>
#include <memory>
#include <vector>

class Publisher {
public:
  static std::unique_ptr<Publisher> create(const char* url);

  explicit Publisher(int fd);

  bool hasSubscribers();

  void publish(const std::vector<std::complex<float> >& samples);
  void publish(const std::vector<int8_t>& samples);
  void publish(const std::array<uint8_t, 892>& packet);

protected:
  int fd_;

  std::vector<std::complex<int8_t> > tmp_;
};
