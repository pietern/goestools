#pragma once

#include <memory>

#include "soft_bit_publisher.h"
#include "types.h"

class Quantize {
public:
  explicit Quantize();

  void setSoftBitPublisher(std::unique_ptr<SoftBitPublisher> softBitPublisher) {
    softBitPublisher_ = std::move(softBitPublisher);
  }

  void work(
      const std::shared_ptr<Queue<std::vector<std::complex<float> > > >& qin,
      const std::shared_ptr<Queue<std::vector<int8_t> > >& qout);

protected:
  std::unique_ptr<SoftBitPublisher> softBitPublisher_;
};
