#pragma once

#include "types.h"

class ClockRecovery {
public:
  explicit ClockRecovery(uint32_t sampleRate, uint32_t symbolRate);

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
};
