#include "packetizer.h"

#include <cstring>
#include <iostream>

Packetizer::Packetizer(std::unique_ptr<Reader> reader)
  : reader_(std::move(reader)) {
  // Include frame prelude at the beginning of the buffer so there
  // is always some preceding data for Viterbi decoder warmup.
  // The first couple of symbols are not passed down to the decoder
  // output, so we have to be able to discard them.
  len_ = encodedFramePreludeBits + encodedFrameBits + encodedSyncWordBits;
  buf_ = static_cast<uint8_t*>(malloc(len_));
  pos_ = 0;
  lock_ = false;
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
  assert(rv == nbytes);
  gpos += nbytes;
  return true;
}

bool Packetizer::nextPacket(std::array<uint8_t, 892>& out, struct timespec* ts) {
  int rv;

  for (;;) {
    auto ok = read();
    if (!ok) {
      return false;
    }

    // If there is a frame lock, don't try and find sync words
    // with better correlation. It is possible that once in a
    // while the frame contains some random sequence that
    // correlates better than our locked position. This then
    // causes unnecessary packet drops.
    //
    // Instead, wait for packet corruption before reacquiring a lock.
    //
    if (!lock_) {
      const auto skip = encodedFramePreludeBits;
      int pos;
      int max = 0;

      // Find position in buffer with maximum correlation with sync word
      pos = correlate(&buf_[skip], len_ - skip, &max, &syncType_);

      // Repeat until we have maximum correlation at 0
      while (pos > 0) {
        std::cerr
          << "Skipping "
          << pos
          << " bits (max. correlation of " << max
          << " for " << correlationTypeToString(syncType_)
          << " at " << pos
          << ")"
          << std::endl;

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
        pos = correlate(&buf_[skip], len_ - skip, &max, &syncType_);
      }

      // Store symbol rate for this stream
      if (syncType_ == HRIT_PHASE_000 || syncType_ == HRIT_PHASE_180) {
        symbolRate_ = 927000;
      } else if (syncType_ == LRIT_PHASE_000 || syncType_ == LRIT_PHASE_180) {
        symbolRate_ = 293883;
      } else {
        assert(false);
      }
    }

    std::array<uint8_t, framePreludeBytes + frameBytes> packet;
    auto bits = encodedFramePreludeBits + encodedFrameBits;
    viterbi_.decodeSoft(&buf_[0], bits , packet.data());

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
    if (rv == -1) {
      lock_ = false;
      std::cerr << "RS unable to correct packet; dropping!" << std::endl;
      continue;
    }

    // Log corrections
    if (rv > 0) {
      std::cerr << "RS corrected " << rv << " bytes" << std::endl;
    }

    // We have a packet!
    lock_ = true;
    break;
  }

  // Include relative time of packet from start of packetizer.
  if (ts != nullptr) {
    auto pos = gpos - (encodedFrameBits + encodedSyncWordBits);
    ts->tv_nsec = (1000000000 * (pos % symbolRate_)) / symbolRate_;
    ts->tv_sec = pos / symbolRate_;
  }

  return true;
}
