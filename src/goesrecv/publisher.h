#pragma once

#include <complex>
#include <memory>
#include <vector>

class Publisher {
public:
  static int bind(const std::vector<std::string>& endpoints);

  static int bind(const std::string& endpoint) {
    std::vector<std::string> endpoints = { endpoint };
    return bind(endpoints);
  }

  static std::unique_ptr<Publisher> create(const std::string& endpoint);

  explicit Publisher(int fd);
  virtual ~Publisher();

  void setSendBuffer(int size);

  bool hasSubscribers();

protected:
  int fd_;
};
