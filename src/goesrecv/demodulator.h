#pragma once

#include <thread>

#include "agc.h"
#include "clock_recovery.h"
#include "config.h"
#include "costas.h"
#include "publisher.h"
#include "quantize.h"
#include "rrc.h"
#include "source.h"
#include "stats_publisher.h"
#include "types.h"

class Demodulator {
public:
  enum Type {
    LRIT = 1,
    HRIT = 2,
  };

  explicit Demodulator(Type t);

  void initialize(Config& config);

  std::shared_ptr<Queue<std::vector<int8_t> > > getSoftBitsQueue() {
    return softBitsQueue_;
  }

  void start();
  void stop();

protected:
  void publishStats();

  uint32_t symbolRate_;
  uint32_t sampleRate_;

  std::unique_ptr<Source> source_;
  std::unique_ptr<StatsPublisher> statsPublisher_;
  std::thread thread_;

  // DSP blocks
  std::unique_ptr<AGC> agc_;
  std::unique_ptr<Costas> costas_;
  std::unique_ptr<RRC> rrc_;
  std::unique_ptr<ClockRecovery> clockRecovery_;
  std::unique_ptr<Quantize> quantization_;

  // Queues
  std::shared_ptr<Queue<Samples> > sourceQueue_;
  std::shared_ptr<Queue<Samples> > agcQueue_;
  std::shared_ptr<Queue<Samples> > costasQueue_;
  std::shared_ptr<Queue<Samples> > rrcQueue_;
  std::shared_ptr<Queue<Samples> > clockRecoveryQueue_;
  std::shared_ptr<Queue<std::vector<int8_t> > > softBitsQueue_;
};
