#pragma once

#include <complex>
#include <memory>
#include <vector>

class Publisher {
public:
  static int bind(const char* url);

  static std::unique_ptr<Publisher> create(const char* url);

  explicit Publisher(int fd);
  virtual ~Publisher();

  void setSendBuffer(int size);

  bool hasSubscribers();

protected:
  int fd_;
};
