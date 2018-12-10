#include "packet_publisher.h"

#include <array>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <util/error.h>

std::unique_ptr<PacketPublisher> PacketPublisher::create(const std::string& endpoint) {
  auto fd = Publisher::bind(endpoint);
  return std::make_unique<PacketPublisher>(fd);
}

PacketPublisher::PacketPublisher(int fd)
  : Publisher(fd) {
}

PacketPublisher::~PacketPublisher() {
}

void PacketPublisher::publish(const std::array<uint8_t, 892>& packet) {
  if (!hasSubscribers()) {
    return;
  }

  auto rv = nn_send(fd_, packet.data(), packet.size() * sizeof(packet[0]), 0);
  if (rv < 0) {
    fprintf(stderr, "nn_send: %s\n", nn_strerror(nn_errno()));
    ASSERT(false);
  }
}
