#include "sample_publisher.h"

#include <cmath>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <util/error.h>

std::unique_ptr<SamplePublisher> SamplePublisher::create(const std::string& endpoint) {
  auto fd = Publisher::bind(endpoint);
  return std::make_unique<SamplePublisher>(fd);
}

SamplePublisher::SamplePublisher(int fd)
    : Publisher(fd) {
}

SamplePublisher::~SamplePublisher() {
}

void SamplePublisher::publish(const Samples& samples) {
  if (!hasSubscribers()) {
    return;
  }

  // Scale samples to 8 bit.
  // Otherwise it is impossible to stream 3M complex samples/second from a RPi.
  tmp_.resize(samples.size());
  for (size_t i = 0; i < samples.size(); i++) {
    auto si = samples[i].real() * 127;
    auto sq = samples[i].imag() * 127;

    // Clamp
    si = (0.5f * (fabsf(si + 127.0f) - fabsf(si - 127.0f)));
    sq = (0.5f * (fabsf(sq + 127.0f) - fabsf(sq - 127.0f)));

    // Convert to int8_t
    tmp_[i].real((int8_t) si);
    tmp_[i].imag((int8_t) sq);
  }

  auto rv = nn_send(fd_, tmp_.data(), tmp_.size() * sizeof(tmp_[0]), 0);
  if (rv < 0) {
    fprintf(stderr, "nn_send: %s\n", nn_strerror(nn_errno()));
    ASSERT(false);
  }
}
