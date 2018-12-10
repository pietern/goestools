#include "nanomsg_writer.h"

#include <sstream>
#include <stdexcept>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <util/error.h>

NanomsgWriter::NanomsgWriter(const std::vector<std::string>& endpoints) {
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

  int size = 1048576;
  auto rv = nn_setsockopt(fd_, NN_SOL_SOCKET, NN_SNDBUF, &size, sizeof(size));
  if (rv < 0) {
    nn_close(fd_);
    std::stringstream ss;
    ss << "nn_setsockopt: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }

  fd_ = fd;
}

NanomsgWriter::~NanomsgWriter() {
  if (fd_ >= 0) {
    nn_close(fd_);
    fd_ = -1;
  }
}

void NanomsgWriter::write(
    const std::array<uint8_t, 892>& in,
    time_t /* unused */) {
  auto rv = nn_send(fd_, in.data(), in.size() * sizeof(in[0]), 0);
  if (rv < 0) {
    std::stringstream ss;
    ss << "nn_send: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }
}
