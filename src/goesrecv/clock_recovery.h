#pragma once

#include <memory>

#include "sample_publisher.h"
#include "types.h"

class ClockRecovery {
public:
  explicit ClockRecovery(uint32_t sampleRate, uint32_t symbolRate);

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  // See http://www.trondeau.com/blog/2011/8/13/control-loop-gain-values.html
  void setLoopBandwidth(float bw);

  // Returns number of samples per symbol.
  float getOmega() const {
    return omega_;
  }

  void work(
      const std::shared_ptr<Queue<Samples> >& qin,
      const std::shared_ptr<Queue<Samples> >& qout);

protected:
  float omega_;
  float omegaMin_;
  float omegaMax_;
  float omegaGain_;
  float mu_;
  float muGain_;

  // Past samples
  std::complex<float> p0t_;
  std::complex<float> p1t_;
  std::complex<float> p2t_;

  // Past associated quadrants
  std::complex<float> c0t_;
  std::complex<float> c1t_;
  std::complex<float> c2t_;

  Samples tmp_;

  std::unique_ptr<SamplePublisher> samplePublisher_;
};
