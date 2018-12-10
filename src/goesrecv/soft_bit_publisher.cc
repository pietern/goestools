#include "soft_bit_publisher.h"

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <util/error.h>

std::unique_ptr<SoftBitPublisher> SoftBitPublisher::create(const std::string& endpoint) {
  auto fd = Publisher::bind(endpoint);
  return std::make_unique<SoftBitPublisher>(fd);
}

SoftBitPublisher::SoftBitPublisher(int fd)
  : Publisher(fd) {
}

SoftBitPublisher::~SoftBitPublisher() {
}

void SoftBitPublisher::publish(const std::vector<int8_t>& bits) {
  if (!hasSubscribers()) {
    return;
  }

  auto rv = nn_send(fd_, bits.data(), bits.size() * sizeof(bits[0]), 0);
  if (rv < 0) {
    fprintf(stderr, "nn_send: %s\n", nn_strerror(nn_errno()));
    ASSERT(false);
  }
}
