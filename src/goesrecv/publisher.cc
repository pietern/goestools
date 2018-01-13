#include "publisher.h"

#include <cassert>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

std::unique_ptr<Publisher> Publisher::create(const char* url) {
  auto fd = nn_socket(AF_SP, NN_PUB);
  if (fd < 0) {
    fprintf(stderr, "nn_socket: %s\n", nn_strerror(nn_errno()));
    assert(false);
  }

  auto rv = nn_bind(fd, url);
  if (rv < 0) {
    fprintf(stderr, "nn_bind: %s\n", nn_strerror(nn_errno()));
    nn_close(fd);
    assert(false);
  }

  return std::make_unique<Publisher>(fd);
}

Publisher::Publisher(int fd) : fd_(fd) {
}

bool Publisher::hasSubscribers() {
  uint32_t subs = (uint32_t) nn_get_statistic(fd_, NN_STAT_CURRENT_CONNECTIONS);
  return subs > 0;
}

void Publisher::publish(const std::vector<std::complex<float> >& samples) {
  if (!hasSubscribers()) {
    return;
  }

  // Scale samples to 8 bit.
  // Otherwise it is impossible to stream 3M complex samples/second from a RPi.
  tmp_.resize(samples.size());
  for (size_t i = 0; i < samples.size(); i++) {
    tmp_[i].real(samples[i].real() * 127);
    tmp_[i].imag(samples[i].imag() * 127);
  }

  auto rv = nn_send(fd_, tmp_.data(), tmp_.size() * sizeof(tmp_[0]), 0);
  if (rv < 0) {
    fprintf(stderr, "nn_send: %s\n", nn_strerror(nn_errno()));
    assert(false);
  }
}
