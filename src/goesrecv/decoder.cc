#include "decoder.h"

#include <pthread.h>

#include <cstring>

#include <util/time.h>

using namespace util;

namespace {

// QueueReader bridges the queue that produces the soft bits
// output of the demodulator to the packetizer.
//
// The packetizer needs an interface that quacks like read(2).
//
class QueueReader : public decoder::Reader {
public:
  explicit QueueReader(std::shared_ptr<Queue<std::vector<int8_t> > > queue)
      : queue_(std::move(queue)) {
  }

  virtual ~QueueReader() {
    // Return read buffer if needed
    if (tmp_) {
      queue_->pushRead(std::move(tmp_));
    }
  }

  virtual size_t read(void* buf, size_t count) {
    char* ptr = (char*) buf;
    size_t nread = 0;
    while (nread < count) {
      // Acquire new read buffer if we don't already have one
      if (!tmp_) {
        tmp_ = queue_->popForRead();
        if (!tmp_) {
          // Can't complete read when queue has closed.
          return 0;
        }
        pos_ = 0;
      }

      auto left = tmp_->size() - pos_;
      auto needed = count - nread;
      auto min = std::min(left, needed);
      memcpy(&ptr[nread], &(*tmp_)[pos_], min);

      // Advance cursors
      nread += min;
      pos_ += min;

      // Return read buffer if it was exhausted
      if (pos_ == tmp_->size()) {
        queue_->pushRead(std::move(tmp_));
      }
    }

    return nread;
  }

protected:
  std::shared_ptr<Queue<std::vector<int8_t> > > queue_;
  std::unique_ptr<std::vector<int8_t> > tmp_;
  size_t pos_;
};

} // namespace

Decoder::Decoder(std::shared_ptr<Queue<std::vector<int8_t> > > queue) {
  packetizer_ = std::make_unique<decoder::Packetizer>(
    std::make_shared<QueueReader>(std::move(queue)));
}

void Decoder::initialize(Config& config) {
  packetPublisher_ = std::move(config.decoder.packetPublisher);
  statsPublisher_ = StatsPublisher::create(config.decoder.statsPublisher.bind);
  if (config.demodulator.statsPublisher.sendBuffer > 0) {
    statsPublisher_->setSendBuffer(config.decoder.statsPublisher.sendBuffer);
  }
}

void Decoder::publishStats(decoder::Packetizer::Details details) {
  if (!statsPublisher_) {
    return;
  }

  const auto timestamp = stringTime();

  std::stringstream ss;
  ss << "{";
  ss << "\"timestamp\": \"" << timestamp << "\",";
  ss << "\"skipped_symbols\": " << details.skippedSymbols << ",";
  ss << "\"viterbi_errors\": " << details.viterbiBits << ",";
  ss << "\"reed_solomon_errors\": " << details.reedSolomonBytes << ",";
  ss << "\"ok\": " << details.ok;
  ss << "}\n";
  statsPublisher_->publish(ss.str());
}

void Decoder::start() {
  thread_ = std::thread([&] {
      std::array<uint8_t, 892> buf;
      decoder::Packetizer::Details details;
      while (packetizer_->nextPacket(buf, &details)) {
        if (details.ok && packetPublisher_) {
          packetPublisher_->publish(buf);
        }
        publishStats(details);
      }
    });
#ifdef __APPLE__
  pthread_setname_np("decoder");
#else
  pthread_setname_np(thread_.native_handle(), "decoder");
#endif
}

void Decoder::stop() {
  thread_.join();
}
