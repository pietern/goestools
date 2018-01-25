#pragma once

extern "C" {
#include <correct.h>
}

#include <array>

namespace decoder {

class ReedSolomon {
public:
  ReedSolomon();
  ~ReedSolomon();

  int run(const uint8_t* data, size_t len, uint8_t* dst);

protected:
  std::array<uint8_t, 256> dualToConv_;
  std::array<uint8_t, 256> convToDual_;

  correct_reed_solomon* rs_;
};

} // namespace decoder
