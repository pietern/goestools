#pragma once

#include <memory>

#include "sample_publisher.h"
#include "types.h"

class RRC {
public:
  static constexpr size_t NTAPS = 31;

  explicit RRC(int df, int sampleRate, int symbolRate);

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  void work(
      const std::shared_ptr<Queue<Samples> >& qin,
      const std::shared_ptr<Queue<Samples> >& qout);

protected:
  int decimation_;
  std::array<float, NTAPS + 1> taps_;

  Samples tmp_;

  std::unique_ptr<SamplePublisher> samplePublisher_;
};
