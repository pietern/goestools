#include "publisher.h"

#include <sstream>
#include <stdexcept>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

int Publisher::bind(const std::vector<std::string>& endpoints) {
  auto fd = nn_socket(AF_SP, NN_PUB);
  if (fd < 0) {
    std::stringstream ss;
    ss << "nn_socket: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }

  for (const auto& endpoint : endpoints) {
    auto rv = nn_bind(fd, endpoint.c_str());
    if (rv < 0) {
      nn_close(fd);
      std::stringstream ss;
      ss << "nn_bind: " << nn_strerror(nn_errno());
      ss << " (" << endpoint << ")";
      throw std::runtime_error(ss.str());
    }
  }

  return fd;
}

std::unique_ptr<Publisher> Publisher::create(const std::string& endpoint) {
  auto fd = Publisher::bind(endpoint);
  return std::make_unique<Publisher>(fd);
}

Publisher::Publisher(int fd) : fd_(fd) {
}

Publisher::~Publisher() {
}

void Publisher::setSendBuffer(int size) {
  int rv = nn_setsockopt(fd_, NN_SOL_SOCKET, NN_SNDBUF, &size, sizeof(size));
  if (rv < 0) {
    nn_close(fd_);
    std::stringstream ss;
    ss << "nn_setsockopt: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }
}

bool Publisher::hasSubscribers() {
  uint32_t subs = (uint32_t) nn_get_statistic(fd_, NN_STAT_CURRENT_CONNECTIONS);
  return subs > 0;
}
