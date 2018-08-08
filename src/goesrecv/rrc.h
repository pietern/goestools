#pragma once

#include <array>
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
  void work(
      size_t nsamples,
      std::complex<float>* fi,
      std::complex<float>* fo);

  int decimation_;
  std::vector<float> taps_;

  Samples tmp_;

  std::unique_ptr<SamplePublisher> samplePublisher_;
};
