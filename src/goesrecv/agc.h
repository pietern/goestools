#pragma once

#include <memory>

#include "types.h"

class AGC {
public:
  explicit AGC();

  void work(
      const std::shared_ptr<Queue<Samples> >& qin,
      const std::shared_ptr<Queue<Samples> >& qout);

protected:
  float gain_;
  float alpha_;
  float acc_;
};
