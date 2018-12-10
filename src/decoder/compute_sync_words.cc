#include <stdint.h>
#include <stdio.h>

#include <array>
#include <vector>

#include <util/error.h>

#include "viterbi.h"

std::array<uint8_t, 4> nrzmEncode(std::array<uint8_t, 4> in, int b) {
  std::array<uint8_t, 4> out;

  // Iterate over bytes
  for (unsigned i = 0; i < in.size(); i++) {
    out[i] = 0;

    // Iterate over bits
    for (unsigned j = 0; j < 8; j++) {
      auto bit = (in[i] >> (7-j)) & 0x1;
      if (bit) {
        // Bit is 1; flip
        b = 1 - b;
      }
      out[i] = (out[i] << 1) | (b & 0x1);
    }
  }
  return out;
}

int main(int argc, char** argv) {
  decoder::Viterbi v;

  // Use array of bytes instead of fundamental type so we
  // don't need to worry about host byte order.
  std::array<uint8_t, 4> syncWord;
  syncWord[0] = 0x1A;
  syncWord[1] = 0xCF;
  syncWord[2] = 0xFC;
  syncWord[3] = 0x1D;

  // LRIT uses NRZ-L "coding" (which is really no coding at all).
  // To get the encoded sync word we can directly Viterbi encode it.
  // Then we can invert it to get its dual to deal with phase
  // ambiguity.
  auto len = v.encodeLength(syncWord.size());
  std::vector<uint8_t> buf(len);
  auto rv = v.encode(syncWord.data(), syncWord.size(), buf.data());
  ASSERT(rv == len);

  // 0 degree phase shift
  printf("LRIT:   phase 0: 0x");
  for (unsigned i = 0; i < 8; i++) {
    printf("%02x", (uint8_t) buf[i]);
  }
  printf("\n");

  // 180 degree phase shift
  printf("LRIT: phase 180: 0x");
  for (unsigned i = 0; i < 8; i++) {
    printf("%02x", (uint8_t) ~buf[i]);
  }
  printf("\n");

  // HRIT uses NRZ-M coding. In this scheme, a 0 means no bit change,
  // and a 1 means a bit change. We have to get an encoded sync word
  // for 2 different initial states to deal with phase ambiguity.
  for (unsigned i = 0; i < 2; i++) {
    auto nrzmSyncWord = nrzmEncode(syncWord, i);
    auto len = v.encodeLength(nrzmSyncWord.size());
    std::vector<uint8_t> buf(len);
    auto rv = v.encode(nrzmSyncWord.data(), nrzmSyncWord.size(), buf.data());
    ASSERT(rv == len);

    if (i == 0) {
      // 0 degree phase shift
      printf("HRIT:   phase 0: 0x");
    } else {
      // 180 degree phase shift
      printf("HRIT: phase 180: 0x");
    }
    for (unsigned i = 0; i < 8; i++) {
      printf("%02x", (uint8_t) buf[i]);
    }
    printf("\n");
  }
}
