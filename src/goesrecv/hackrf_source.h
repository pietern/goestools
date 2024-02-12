#pragma once

#include <libhackrf/hackrf.h>
#include <stdint.h>

#include <memory>
#include <thread>
#include <vector>

#include "source.h"

class HackRF : public Source {
 public:
  static std::unique_ptr<HackRF> open(uint32_t index = 0);

  explicit HackRF(struct hackrf_device* dev);
  ~HackRF();

  std::vector<uint32_t> getSampleRates() const { return sampleRates_; }

  void setFrequency(uint32_t freq);

  void setSampleRate(uint32_t rate);

  virtual uint32_t getSampleRate() const override;

  void setBbGain(int gain);

  void setIfGain(int gain);

  void setRfAmplifier(bool on);

  void setBiasTee(bool on);

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  virtual void start(const std::shared_ptr<Queue<Samples> >& queue) override;

  virtual void stop() override;

  void handle(const hackrf_transfer* transfer);

 protected:
  struct hackrf_device* dev_;

  std::vector<uint32_t> loadSampleRates();
  void process(size_t nsamples, unsigned char* buf, std::complex<float>* fo);
  std::vector<uint32_t> sampleRates_;
  std::uint32_t sampleRate_;

  // Background RX thread
  std::thread thread_;

  // Set on start; cleared on stop
  std::shared_ptr<Queue<Samples> > queue_;

  // Optional publisher for samples
  std::unique_ptr<SamplePublisher> samplePublisher_;
};
