#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace decoder {

class Derandomizer {
public:
  Derandomizer();

  void run(uint8_t* data, size_t len);

protected:
  std::array<uint8_t, 1020> table_;
};

} // namespace decoder
