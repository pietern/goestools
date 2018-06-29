#pragma once

#include <stdint.h>

#include <memory>
#include <thread>
#include <vector>

#include <rtl-sdr.h>

#include "source.h"

class RTLSDR : public Source {
public:
  static std::unique_ptr<RTLSDR> open(uint32_t index = 0);

  explicit RTLSDR(rtlsdr_dev_t* dev);
  ~RTLSDR();

  const std::vector<int>& getTunerGains() {
    return tunerGains_;
  }

  void setFrequency(uint32_t freq);

  void setSampleRate(uint32_t rate);

  virtual uint32_t getSampleRate() const override;

  void setTunerGain(int gain);

  void setBiasTee(bool on);

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  virtual void start(const std::shared_ptr<Queue<Samples> >& queue) override;

  virtual void stop() override;

  void handle(unsigned char* buf, uint32_t len);

protected:
  void process(
      size_t nsamples,
      unsigned char* buf,
      std::complex<float>* fo);

  rtlsdr_dev_t* dev_;

  std::vector<int> tunerGains_;
  std::thread thread_;

  // Set on start; cleared on stop
  std::shared_ptr<Queue<Samples> > queue_;

  // Optional publisher for samples
  std::unique_ptr<SamplePublisher> samplePublisher_;
};
