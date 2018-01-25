#pragma once

#include <cassert>
#include <memory>

#include "correlator.h"
#include "derandomizer.h"
#include "reader.h"
#include "reed_solomon.h"
#include "viterbi.h"

namespace decoder {

class Packetizer {
  static constexpr auto frameBits = 8192;
  static constexpr auto syncWordBits = 32;
  static constexpr auto framePreludeBits = 32;

  // Encoding is twice the size of the original (convolutional code has r=1/2)
  static constexpr auto encodedFrameBits = 2 * frameBits;
  static constexpr auto encodedSyncWordBits = 2 * syncWordBits;
  static constexpr auto encodedFramePreludeBits = 2 * framePreludeBits;

  // For convenience
  static constexpr auto frameBytes = frameBits / 8;
  static constexpr auto syncWordBytes = syncWordBits / 8;
  static constexpr auto framePreludeBytes = framePreludeBits / 8;

public:
  explicit Packetizer(std::shared_ptr<Reader> reader);

  bool nextPacket(std::array<uint8_t, 892>& out, struct timespec* ts);

protected:
  bool read();

  std::shared_ptr<Reader> reader_;
  Viterbi viterbi_;
  Derandomizer derandomizer_;
  ReedSolomon reedSolomon_;

  uint8_t* buf_;
  size_t len_;
  size_t pos_;
  bool lock_;
  correlationType syncType_;
  int symbolRate_;
  int64_t symbolPos_;
};

} // namespace decoder
