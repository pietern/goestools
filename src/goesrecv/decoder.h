#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "decoder/packetizer.h"

#include "config.h"
#include "packet_publisher.h"
#include "queue.h"

class Decoder {
public:
  explicit Decoder(std::shared_ptr<Queue<std::vector<int8_t> > > queue);

  void initialize(Config& config);

  void start();
  void stop();

protected:
  std::unique_ptr<decoder::Packetizer> packetizer_;
  std::unique_ptr<PacketPublisher> packetPublisher_;
  std::thread thread_;
};
