#include "correlator.h"

namespace {

// Sync word after applying convolutional code.
// First one is in phase, second one is 180 degrees out of phase.
const uint64_t encodedSyncWords[2] = {
  0x035d49c24ff2686b,
  0xfca2b63db00d9794,
};

// Signal phase per encoded sync word.
const int phase[2] = {
  0,
  180,
};

const unsigned encodedSyncWordBits = 64;

} // namespace

int correlate(uint8_t* data, size_t len, int* maxOut, int* phaseOut) {
  uint64_t tmp = 0;

  // Position with maximum correlation
  int pos[2] = { 0, 0 };

  // Maximum correlation
  int max[2] = { 0, 0 };

  // Find maximum correlation
  for (unsigned i = 0; i < len - 1; i++) {
    // If data >= 128 (i.e. MSB == 1), set bit in stream.
    tmp = (tmp << 1) | (data[i] & 0x80) >> 7;
    if (i < (encodedSyncWordBits - 1)) {
      continue;
    }

    // Match tmp against encoded sync words
    for (unsigned j = 0; j < 2; j++) {
      auto v = 64 - __builtin_popcount(tmp ^ encodedSyncWords[j]);
      if (v > max[j]) {
        max[j] = v;
        pos[j] = i - (encodedSyncWordBits - 1);
      }
    }
  }

  unsigned i = (max[0] >= max[1]) ? 0 : 1;
  *maxOut = max[i];
  *phaseOut = phase[i];
  return pos[i];
}
