#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "goesdec/packetizer.h"

#include "./publisher.h"
#include "./queue.h"

class Decoder {
public:
  explicit Decoder(std::shared_ptr<Queue<std::vector<int8_t> > > queue);

  void start();
  void stop();

protected:
  std::unique_ptr<Packetizer> packetizer_;
  std::unique_ptr<Publisher> packetPublisher_;
  std::thread thread_;
};
