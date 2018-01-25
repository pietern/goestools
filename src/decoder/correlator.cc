#include "correlator.h"

namespace decoder {

namespace {

// The sync words below are compared to the raw bit stream
// to synchronize it with the start of a new packet.
// See compute_sync_words.cc for more information.
const uint64_t encodedSyncWords[4] = {
  // LRIT sync words
  0x035d49c24ff2686b,
  0xfca2b63db00d9794,
  // HRIT sync words
  0x03b10b02f33d2076,
  0xdafef4fd0cc2df89,
};

} // namespace

const unsigned encodedSyncWordBits = 64;

const char* correlationTypeToString(correlationType t) {
  switch (t) {
  case LRIT_PHASE_000:
    return "LRIT 0 deg";
  case LRIT_PHASE_180:
    return "LRIT 180 deg";
  case HRIT_PHASE_000:
    return "HRIT 0 deg";
  case HRIT_PHASE_180:
    return "HRIT 180 deg";
  }
  return "";
}

int correlate(uint8_t* data, size_t len, int* maxOut, correlationType* maxType) {
  uint64_t tmp = 0;

  // Position with maximum correlation
  int pos[4] = { 0, 0, 0, 0 };

  // Maximum correlation
  int max[4] = { 0, 0, 0, 0 };

  // Find maximum correlation
  for (unsigned i = 0; i < len; i++) {
    // If data >= 128 (i.e. MSB == 1), set bit in stream.
    tmp = (tmp << 1) | (data[i] & 0x80) >> 7;
    if (i < (encodedSyncWordBits - 1)) {
      continue;
    }

    // Match tmp against encoded sync words
    for (unsigned j = 0; j < 4; j++) {
      auto v = 64 - __builtin_popcount(tmp ^ encodedSyncWords[j]);
      if (v > max[j]) {
        max[j] = v;
        pos[j] = i - (encodedSyncWordBits - 1);
      }
    }
  }

  // Return position for best correlating sync word
  int j = 0;
  for (unsigned i = 0; i < 4; i++) {
    if (max[i] > max[j]) {
      j = i;
    }
  }

  if (maxOut != nullptr) {
    *maxOut = max[j];
  }
  if (maxType != nullptr) {
    *maxType = static_cast<correlationType>(j);
  }
  return pos[j];
}

} // namespace decoder
