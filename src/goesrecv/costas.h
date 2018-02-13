#pragma once

#include <memory>

#include "sample_publisher.h"
#include "types.h"

class Costas {
public:
  explicit Costas();

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

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

  std::unique_ptr<SamplePublisher> samplePublisher_;
};
