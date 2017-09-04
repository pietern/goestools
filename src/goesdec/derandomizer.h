#pragma once

#include <stdint.h>

#include <array>
#include <vector>

class Derandomizer {
public:
  Derandomizer();

  void run(uint8_t* data, size_t len);

protected:
  std::array<uint8_t, 1020> table_;
};
