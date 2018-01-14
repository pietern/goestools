#pragma once

#include "types.h"

class RRC {
public:
  static constexpr size_t NTAPS = 31;

  explicit RRC(int df, int sampleRate, int symbolRate);

  void work(
      const std::shared_ptr<Queue<Samples> >& qin,
      const std::shared_ptr<Queue<Samples> >& qout);

protected:
  int decimation_;
  std::array<float, NTAPS + 1> taps_;

  Samples tmp_;
};
