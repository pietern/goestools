#include "nanomsg_reader.h"

#include <cstring>
#include <sstream>
#include <stdexcept>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

NanomsgReader::NanomsgReader(const std::string& addr) {
  int rv;

  auto fd = nn_socket(AF_SP, NN_SUB);
  if (fd < 0) {
    std::stringstream ss;
    ss << "nn_socket: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }

  // Fix receive buffer size to "plenty".
  // HRIT packet stream is 50KB/sec so this gives us 5+ minutes
  // before the buffer is full and nanomsg starts dropping messages.
  int size = 16 * 1024 * 1024;
  rv = nn_setsockopt(fd, NN_SOL_SOCKET, NN_RCVBUF, &size, sizeof(size));
  if (rv < 0) {
    nn_close(fd);
    std::stringstream ss;
    ss << "nn_setsockopt: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }

  rv = nn_connect(fd, addr.c_str());
  if (rv < 0) {
    nn_close(fd);
    std::stringstream ss;
    ss << "nn_connect: " << nn_strerror(nn_errno());
    ss << " (" << addr << ")";
    throw std::runtime_error(ss.str());
  }

  rv = nn_setsockopt(fd, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  if (rv < 0) {
    nn_close(fd);
    std::stringstream ss;
    ss << "nn_setsockopt: " << nn_strerror(nn_errno());
    ss << " (" << addr << ")";
    throw std::runtime_error(ss.str());
  }

  fd_ = fd;
}

NanomsgReader::~NanomsgReader() {
  nn_close(fd_);
}

bool NanomsgReader::nextPacket(std::array<uint8_t, 892>& out) {
  void* buf = nullptr;
  int nbytes;
  for (;;) {
    nbytes = nn_recv(fd_, &buf, NN_MSG, 0);
    if (nbytes < 0) {
      std::stringstream ss;
      ss << "nn_recv: " << nn_strerror(nn_errno());
      throw std::runtime_error(ss.str());
    }
    if (nbytes != (int) out.size()) {
      continue;
    }

    memcpy(out.data(), buf, nbytes);
    nn_freemsg(buf);
    return true;
  }
}
