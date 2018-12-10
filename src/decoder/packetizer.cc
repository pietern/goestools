#include "packetizer.h"

#include <cstring>
#include <iostream>

#include <util/error.h>

namespace decoder {

Packetizer::Packetizer(std::shared_ptr<Reader> reader)
  : reader_(std::move(reader)) {
  // Include frame prelude at the beginning of the buffer so there
  // is always some preceding data for Viterbi decoder warmup.
  // The first couple of symbols are not passed down to the decoder
  // output, so we have to be able to discard them.
  len_ = encodedFramePreludeBits + encodedFrameBits + encodedSyncWordBits;
  buf_ = static_cast<uint8_t*>(malloc(len_));
  pos_ = 0;
  lock_ = false;
  symbolPos_ = 0;
}

bool Packetizer::read()  {
  int rv;
  int nbytes = len_ - pos_;
  rv = reader_->read(buf_ + pos_, nbytes);
  if (rv == -1) {
    perror("read");
    exit(1);
  }
  if (rv == 0) {
    return false;
  }
  ASSERT(rv == nbytes);
  symbolPos_ += nbytes;
  return true;
}

bool Packetizer::nextPacket(std::array<uint8_t, 892>& out, Details* details) {
  int rv;

  // Initialize accumulation fields
  if (details) {
    details->skippedSymbols = 0;
  }

  for (;;) {
    auto ok = read();
    if (!ok) {
      return false;
    }

    // If there is a frame lock, only run correlation detector against
    // the sync word itself. This will ensure that we catch phase
    // flips that happen so quickly that bit errors can be corrected.
    // Note that this typically only happens if the signal demodulator
    // it too jittery. To make it less jittery, it can help to reduce
    // the loop bandwidth of the carrier tracking loop (Costas Loop).
    //
    // Only do this for LRIT because HRIT doesn't have this ambiguity.
    //
    // We don't run the correlation detector against the whole frame
    // if there is a lock. It is possible that once in a while the
    // frame contains some random sequence that correlates better than
    // our locked position. This then causes unnecessary packet drops.
    //
    // Instead, wait for packet corruption before reacquiring a lock.
    //
    if (lock_ && (syncType_ == LRIT_PHASE_000 || syncType_ == LRIT_PHASE_180)) {
      const auto skip = encodedFramePreludeBits;
      auto prevSyncType = syncType_;
      correlate(&buf_[skip], encodedSyncWordBits, nullptr, &syncType_);
      if (syncType_ != prevSyncType) {
        std::cerr
          << "Phase flip detected"
          << " from "<< correlationTypeToString(prevSyncType)
          << " to " << correlationTypeToString(syncType_)
          << std::endl;
      }
    }

    // Reacquire lock
    if (!lock_) {
      const auto skip = encodedFramePreludeBits;
      int pos;
      int max = 0;

      // Repeat until we have maximum correlation at 0
      for (;;) {
        // Find position in buffer with maximum correlation with sync word
        pos = correlate(&buf_[skip], len_ - skip, &max, &syncType_);

        // If the current position is the one with the best correlation OR
        // the position exactly one frame away, assume we're OK.
        if (pos == 0 || pos == encodedFrameBits) {
          break;
        }

        // Keep track of the number of skipped symbols
        if (details) {
          details->skippedSymbols += pos;
        }

        // Skip over chunk that didn't qualify and try again,
        // while keeping the frame prelude for the aspiring frame.
        // The position in "pos" refers to the index with maximum
        // correlation offset by encodedFramePreludeBits. This
        // means the memmove below includes the frame prelude.
        memmove(buf_, buf_ + pos, len_ - pos);
        pos_ = len_ - pos;

        // Fill tail and correlate again
        ok = read();
        if (!ok) {
          return false;
        }
      }

      // Store symbol rate for this stream
      if (syncType_ == HRIT_PHASE_000 || syncType_ == HRIT_PHASE_180) {
        symbolRate_ = 927000;
      } else if (syncType_ == LRIT_PHASE_000 || syncType_ == LRIT_PHASE_180) {
        symbolRate_ = 293883;
      } else {
        ASSERT(false);
      }
    }

    std::array<uint8_t, framePreludeBytes + frameBytes> packet;
    auto bits = encodedFramePreludeBits + encodedFrameBits;
    viterbi_.decodeSoft(&buf_[0], bits , packet.data());

    // Re-code packet to compute number of Viterbi corrected bits
    if (details) {
      details->viterbiBits = viterbi_.compareSoft(&buf_[0], packet.data(), packet.size());
    }

    // Move tail bits of read buffer to beginning.
    // This includes a new prelude, which is equal to the
    // last bits of the current frame.
    auto tail = encodedFramePreludeBits + encodedSyncWordBits;
    memmove(buf_, buf_ + len_ - tail, tail);
    pos_ = tail;

    // If maximum correlation was found for an out of phase
    // LRIT sync word, negate packet to make it in-phase.
    // We can do this after Viterbi because it works just as
    // well for negated signals. It just yields negated output.
    if (syncType_ == LRIT_PHASE_180) {
      for (unsigned i = 0; i < packet.size(); i++) {
        packet[i] ^= 0xff;
      }
    }

    // If maximum correlation was found for an HRIT sync word,
    // run NRZ-M decoder on the bit stream.
    if (syncType_ == HRIT_PHASE_000 || syncType_ == HRIT_PHASE_180) {
      // An NRZ-M encoder performs a bit wise: o[i+1] = in[i] ^ o[i].
      // Hence, for the decoder we perform: in[i] = o[i+1] ^ o[i].
      uint8_t b0 = 0;
      uint8_t m;
      auto data = packet.data();
      for (unsigned i = 0; i < packet.size(); i++) {
        m = (b0 << 7) | ((data[i] >> 1) & 0x7f);
        b0 = data[i] & 0x1;
        data[i] ^= m;
      }
    }

    uint32_t syncWord;
    memcpy(&syncWord, &packet[framePreludeBytes], syncWordBytes);

    // Discard the warmup frame prelude and sync word.
    auto skip = framePreludeBytes + syncWordBytes;
    memmove(&packet[0], &packet[skip], packet.size() - skip);

    // De-randomize packet
    auto len = frameBytes - syncWordBytes;
    derandomizer_.run(&packet[0], len);

    // Reed-Solomon
    rv = reedSolomon_.run(&packet[0], len, &out[0]);

    // Log corrections
    // This is -1 if it was not correctable
    if (details) {
      details->reedSolomonBytes = rv;
    }

    // We have a lock if this packet was correctable
    lock_ = (rv >= 0);
    if (details) {
      details->ok = lock_;
    }
    break;
  }

  // Include relative time of packet from start of packetizer.
  if (details != nullptr) {
    auto pos = symbolPos_ - (encodedFrameBits + encodedSyncWordBits);
    details->symbolPos = pos;
    details->relativeTime.tv_nsec = (1000000000 * (pos % symbolRate_)) / symbolRate_;
    details->relativeTime.tv_sec = pos / symbolRate_;
  }

  return true;
}

} // namespace decoder
