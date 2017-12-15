#include <assert.h>
#include <malloc.h>
#include <string.h>

#include <array>
#include <iostream>
#include <map>

#include "correlator.h"
#include "derandomizer.h"
#include "file_handler.h"
#include "packet.h"
#include "reader.h"
#include "reed_solomon.h"
#include "vcdu.h"
#include "virtual_channel.h"
#include "viterbi.h"

class Decoder {
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
  explicit Decoder(std::unique_ptr<Reader> reader)
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

  void read() {
    int rv;
    int nbytes = len_ - pos_;
    rv = reader_->read(buf_ + pos_, nbytes);
    if (rv == -1) {
      perror("read");
      exit(1);
    }
    assert(rv == nbytes);
    gpos += nbytes;
  }

  void nextPacket(std::array<uint8_t, 892>& out) {
    int rv;

    for (;;) {
      read();

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
          read();
          pos = correlate(&buf_[skip], len_ - skip, &max, &syncType_);
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
  }

protected:
  std::unique_ptr<Reader> reader_;
  Viterbi viterbi_;
  Derandomizer derandomizer_;
  ReedSolomon reedSolomon_;

  uint8_t* buf_;
  size_t len_;
  size_t pos_;
  bool lock_;
  correlationType syncType_;

  int gpos = 0;
};

int main(int argc, char** argv) {
  // Turn argv into list of files
  std::vector<std::string> files;
  for (int i = 1; i < argc; i++) {
    files.push_back(argv[i]);
  }

  FileHandler handler("./out");
  Decoder d(std::make_unique<MultiFileReader>(files));
  std::array<uint8_t, 892> buf;

  std::map<int, VirtualChannel> vcs;
  for (auto j = 0; ; j++) {
    d.nextPacket(buf);
    VCDU vcdu(buf);

    // Ignore fill packets
    auto vcid = vcdu.getVCID();
    if (vcid == 63) {
      continue;
    }

    // Create virtual channel instance if it does not yet exist
    if (vcs.find(vcid) == vcs.end()) {
      vcs.insert(std::make_pair(vcid, VirtualChannel(vcid)));
    }

    // Let virtual channel process VCDU
    auto it = vcs.find(vcid);
    auto spdus = it->second.process(vcdu);
    for (auto& spdu : spdus) {
      handler.handle(std::move(spdu));
    }
  }
}
