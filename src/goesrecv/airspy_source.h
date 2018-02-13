#pragma once

#include <stdint.h>

#include <memory>
#include <thread>
#include <vector>

#include <libairspy/airspy.h>

#include "sample_publisher.h"
#include "types.h"

class Airspy {
public:
  static std::unique_ptr<Airspy> open(uint32_t index = 0);

  explicit Airspy(struct airspy_device* dev);
  ~Airspy();

  std::vector<uint32_t> getSampleRates() const {
    return sampleRates_;
  }

  void setCenterFrequency(uint32_t freq);
  void setSampleRate(uint32_t rate);
  void setGain(int gain);

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  void start(const std::shared_ptr<Queue<Samples> >& queue);
  void stop();
  void handle(const airspy_transfer* transfer);

protected:
  struct airspy_device* dev_;

  std::vector<uint32_t> loadSampleRates();
  std::vector<uint32_t> sampleRates_;

  // Background RX thread
  std::thread thread_;

  // Set on start; cleared on stop
  std::shared_ptr<Queue<Samples> > queue_;

  // Optional publisher for samples
  std::unique_ptr<SamplePublisher> samplePublisher_;
};
