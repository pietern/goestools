#pragma once

#include "types.h"

class Costas {
public:
  explicit Costas();

  // Returns frequency in radians per sample.
  float getFrequency() const {
    return freq_;
  }

  void work(
      const std::shared_ptr<Queue<Samples> >& qin,
      const std::shared_ptr<Queue<Samples> >& qout);

protected:
  float phase_;
  float freq_;
  float alpha_;
  float beta_;
};
