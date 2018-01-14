#pragma once

#include "types.h"

class Quantize {
public:
  explicit Quantize();

  void work(
      const std::shared_ptr<Queue<std::vector<std::complex<float> > > >& qin,
      const std::shared_ptr<Queue<std::vector<int8_t> > >& qout);
};
