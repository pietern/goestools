#pragma once

#include <memory>

#include "sample_publisher.h"
#include "types.h"

class AGC {
public:
  explicit AGC();

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  void work(
      const std::shared_ptr<Queue<Samples> >& qin,
      const std::shared_ptr<Queue<Samples> >& qout);

protected:
  void work(
      size_t nsamples,
      std::complex<float>* fi,
      std::complex<float>* fo);

  float gain_;
  float alpha_;
  float acc_;

  std::unique_ptr<SamplePublisher> samplePublisher_;
};
