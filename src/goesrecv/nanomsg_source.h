#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "config.h"
#include "sample_publisher.h"
#include "types.h"

class Nanomsg {
public:
  static std::unique_ptr<Nanomsg> open(const Config& config);

  explicit Nanomsg(int fd);
  ~Nanomsg();

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  void start(const std::shared_ptr<Queue<Samples> >& queue);
  void stop();

protected:
  void loop();

  int fd_;
  std::thread thread_;

  // Set on start; cleared on stop
  std::shared_ptr<Queue<Samples> > queue_;

  // Optional publisher for samples
  std::unique_ptr<SamplePublisher> samplePublisher_;
};
