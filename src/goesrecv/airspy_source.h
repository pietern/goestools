#pragma once

#include <stdint.h>

#include <memory>
#include <thread>
#include <vector>

#include <libairspy/airspy.h>

#include "source.h"

class Airspy : public Source {
public:
  static std::unique_ptr<Airspy> open(uint32_t index = 0);

  explicit Airspy(struct airspy_device* dev);
  ~Airspy();

  std::vector<uint32_t> getSampleRates() const {
    return sampleRates_;
  }

  void setFrequency(uint32_t freq);

  void setSampleRate(uint32_t rate);

  virtual uint32_t getSampleRate() const override;

  void setGain(int gain);

  void setBiasTee(bool on);

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  virtual void start(const std::shared_ptr<Queue<Samples> >& queue) override;

  virtual void stop() override;

  void handle(const airspy_transfer* transfer);

protected:
  struct airspy_device* dev_;

  std::vector<uint32_t> loadSampleRates();
  std::vector<uint32_t> sampleRates_;
  std::uint32_t sampleRate_;

  // Background RX thread
  std::thread thread_;

  // Set on start; cleared on stop
  std::shared_ptr<Queue<Samples> > queue_;

  // Optional publisher for samples
  std::unique_ptr<SamplePublisher> samplePublisher_;
};
