#include "sample_publisher.h"

#include <cassert>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

std::unique_ptr<SamplePublisher> SamplePublisher::create(const std::string& url) {
  auto fd = Publisher::bind(url.c_str());
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
    tmp_[i].real(samples[i].real() * 127);
    tmp_[i].imag(samples[i].imag() * 127);
  }

  auto rv = nn_send(fd_, tmp_.data(), tmp_.size() * sizeof(tmp_[0]), 0);
  if (rv < 0) {
    fprintf(stderr, "nn_send: %s\n", nn_strerror(nn_errno()));
    assert(false);
  }
}
