#pragma once

#include <stdint.h>
#include <unistd.h>

#include <string>

namespace decoder {

enum correlationType {
  LRIT_PHASE_000 = 0,
  LRIT_PHASE_180 = 1,
  HRIT_PHASE_000 = 2,
  HRIT_PHASE_180 = 3,
};

// Number of bits used by sync word
extern const unsigned encodedSyncWordBits;

const char* correlationTypeToString(correlationType type);

// Correlates bit stream with sync words.
// This function is only used when there is no signal lock, so we can
// afford to correlate with both LRIT and HRIT sync words. Doing this
// means we don't need a run time flag for the type of stream because
// we can detect which one we correlate best with.
int correlate(uint8_t* data, size_t len, int* maxOut, correlationType* maxType);

} // namespace decoder
