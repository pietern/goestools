#pragma once

#include <memory>

#include "sample_publisher.h"
#include "types.h"

class AGC {
public:
  explicit AGC();

  void setMin(float min) {
    min_ = min;
  }

  void setMax(float max) {
    max_ = max;
  }

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  float getGain() const {
    return gain_;
  }

  void work(
      const std::shared_ptr<Queue<Samples> >& qin,
      const std::shared_ptr<Queue<Samples> >& qout);

protected:
  void work(
      size_t nsamples,
      std::complex<float>* fi,
      std::complex<float>* fo);

  float min_;
  float max_;
  float gain_;
  float alpha_;

  std::unique_ptr<SamplePublisher> samplePublisher_;
};
