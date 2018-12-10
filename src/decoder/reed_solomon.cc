#include "reed_solomon.h"

#include <util/error.h>

namespace decoder {

namespace {

// Column-by-column representation of T_{\alpha\ell}.
// See https://public.ccsds.org/Pubs/101x0b6s.pdf, Annex A.
const uint32_t tal[] = {
  0b11111110,
  0b01101001,
  0b01101011,
  0b00001101,
  0b11101111,
  0b11110010,
  0b01011011,
  0b11000111,
};

} // namespace

ReedSolomon::ReedSolomon() {
  // Initialize lookup tables to convert between conventional and dual
  // basis representation. The Reed-Solomon implementation in libcorrect
  // uses conventional representation, yet the data we process uses dual
  // basis representation. We can apply direct conversion between the
  // two to use the libcorrect implementation for dual basis data.

  // For every symbol
  for (int i = 0; i < 256; i++) {
    convToDual_[i] = 0;

    // Bit-by-bit multiply is AND.
    // Output bit is sum of the bit-by-bit multiplications mod 2 (popcount() & 0x1)
    for (int j = 0; j < 8; j++) {
      int v = (__builtin_popcount(i & tal[j]) & 0x1);
      convToDual_[i] |= v << (7-j);
    }

    // Inverse mapping
    dualToConv_[convToDual_[i]] = i;
  }

  // Initialize Reed-Solomon decoder
  rs_ = correct_reed_solomon_create(
    correct_rs_primitive_polynomial_ccsds,
    112,
    11,
    32);
}

ReedSolomon::~ReedSolomon() {
  correct_reed_solomon_destroy(rs_);
}

int ReedSolomon::run(const uint8_t* data, size_t len, uint8_t* dst) {
  std::array<uint8_t, 255> tmp1, tmp2;
  int err = 0;

  // Expect 4x 255 byte block (223 data + 32 parity)
  ASSERT(len == 1020);

  // Process block by block
  for (auto i = 0; i < 4; i++) {
    // Deinterleave and convert
    for (auto j = 0; j < 255; j++) {
      tmp1[j] = dualToConv_[data[(j * 4) + i]];
    }

    // Run Reed-Solomon (in conventional representation)
    auto rv = correct_reed_solomon_decode(rs_, tmp1.data(), tmp1.size(), tmp2.data());
    if (rv == -1) {
      return -1;
    }

    // Count number of corrected errors
    for (auto j = 0; j < (255 - 32); j++) {
      if (tmp1[j] != tmp2[j]) {
        err++;
      }
    }

    // Convert and interleave (ignoring parity)
    for (auto j = 0; j < (255 - 32); j++) {
      dst[(j * 4) + i] = convToDual_[tmp2[j]];
    }
  }

  return err;
}

} // namespace decoder
