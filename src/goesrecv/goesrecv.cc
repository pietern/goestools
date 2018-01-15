#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>

#include <iostream>
#include <memory>
#include <thread>

#include "agc.h"
#include "airspy_source.h"
#include "clock_recovery.h"
#include "costas.h"
#include "options.h"
#include "publisher.h"
#include "quantize.h"
#include "rrc.h"
#include "rtlsdr_source.h"

class Demodulator {
public:
  enum Type {
    LRIT = 1,
    HRIT = 2,
  };

  explicit Demodulator(Type t) {
    switch (t) {
    case LRIT:
      freq_ = 1691000000;
      symbolRate_ = 293883;
      break;
    case HRIT:
      freq_ = 1694100000;
      symbolRate_ = 927000;
      break;
    default:
      assert(false);
    }

    // Sample rate depends on source
    sampleRate_ = 0;

    // Decimation depends on sample rate
    decimationFactor_ = 1;

    // Initialize queues
    // It is possible to attach publishers before starting the demodulator
    sourceQueue_ = std::make_shared<Queue<Samples> >(4);
    agcQueue_ = std::make_shared<Queue<Samples> >(2);
    rrcQueue_ = std::make_shared<Queue<Samples> >(2);
    costasQueue_ = std::make_shared<Queue<Samples> >(2);
    clockRecoveryQueue_ = std::make_shared<Queue<Samples> >(2);
    softBitsQueue_ = std::make_shared<Queue<std::vector<int8_t> > >(2);

    // Initialize publishers
    sourcePublisher_ = Publisher::create("tcp://0.0.0.0:5003");
    sourceQueue_->attach(std::move(sourcePublisher_));
    clockRecoveryPublisher_ = Publisher::create("tcp://0.0.0.0:5002");
    clockRecoveryQueue_->attach(std::move(clockRecoveryPublisher_));
    softBitsPublisher_ = Publisher::create("tcp://0.0.0.0:5001");
    softBitsQueue_->attach(std::move(softBitsPublisher_));
  }

  std::shared_ptr<Queue<std::vector<int8_t> > > getSoftBitsQueue() {
    return softBitsQueue_;
  }

  bool configureSource(const std::string& source) {
    if (source == "airspy") {
      sampleRate_ = 3000000;
      airspy_ = Airspy::open();
      airspy_->setCenterFrequency(freq_);
      airspy_->setSampleRate(sampleRate_);
      airspy_->setGain(18);
      return true;
    }

    if (source == "rtlsdr") {
      sampleRate_ = 2400000;
      rtlsdr_ = RTLSDR::open();
      rtlsdr_->setCenterFrequency(freq_);
      rtlsdr_->setSampleRate(sampleRate_);
      rtlsdr_->setTunerGain(30);
      return true;
    }

    return false;
  }

  void start() {
    auto sr1 = sampleRate_;
    auto sr2 = sampleRate_ / decimationFactor_;
    auto dc = decimationFactor_;

    agc_ = std::make_unique<AGC>();
    rrc_ = std::make_unique<RRC>(dc, sr1, symbolRate_);
    costas_ = std::make_unique<Costas>();
    clockRecovery_ = std::make_unique<ClockRecovery>(sr2, symbolRate_);
    quantization_ = std::make_unique<Quantize>();

    thread_ = std::thread([&] {
        while (!sourceQueue_->closed()) {
          agc_->work(sourceQueue_, agcQueue_);
          rrc_->work(agcQueue_, rrcQueue_);
          costas_->work(rrcQueue_, costasQueue_);
          clockRecovery_->work(costasQueue_, clockRecoveryQueue_);
          quantization_->work(clockRecoveryQueue_, softBitsQueue_);
        }
      });

    if (airspy_) {
      airspy_->start(sourceQueue_);
    }
    if (rtlsdr_) {
      rtlsdr_->start(sourceQueue_);
    }
  }

  void stop() {
    if (airspy_) {
      airspy_->stop();
    }
    if (rtlsdr_) {
      rtlsdr_->stop();
    }

    thread_.join();
  }

protected:
  uint32_t freq_;
  uint32_t symbolRate_;
  uint32_t sampleRate_;
  uint32_t decimationFactor_;

  std::thread thread_;

  // Sources (only one is used)
  std::unique_ptr<Airspy> airspy_;
  std::unique_ptr<RTLSDR> rtlsdr_;

  // DSP blocks
  std::unique_ptr<AGC> agc_;
  std::unique_ptr<RRC> rrc_;
  std::unique_ptr<Costas> costas_;
  std::unique_ptr<ClockRecovery> clockRecovery_;
  std::unique_ptr<Quantize> quantization_;

  // Queues
  std::shared_ptr<Queue<Samples> > sourceQueue_;
  std::shared_ptr<Queue<Samples> > agcQueue_;
  std::shared_ptr<Queue<Samples> > rrcQueue_;
  std::shared_ptr<Queue<Samples> > costasQueue_;
  std::shared_ptr<Queue<Samples> > clockRecoveryQueue_;
  std::shared_ptr<Queue<std::vector<int8_t> > > softBitsQueue_;

  // Publishers (so queues can be monitored externally)
  std::unique_ptr<Publisher> sourcePublisher_;
  std::unique_ptr<Publisher> clockRecoveryPublisher_;
  std::unique_ptr<Publisher> softBitsPublisher_;
};

static bool sigint = false;

static void signalHandler(int signum) {
  fprintf(stderr, "Signal caught, exiting!\n");
  sigint = true;
}

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);

  // Convert string option to enum
  Demodulator::Type downlinkType;
  if (opts.downlinkType == "lrit") {
    downlinkType = Demodulator::LRIT;
  } else if (opts.downlinkType == "hrit") {
    downlinkType = Demodulator::HRIT;
  } else {
    std::cerr << "Invalid downlink type: " << opts.downlinkType << std::endl;
    exit(1);
  }

  Demodulator demod(downlinkType);
  if (!demod.configureSource(opts.device)) {
    std::cerr << "Invalid device: " << opts.device << std::endl;
    exit(1);
  }

  // Install signal handler
  struct sigaction sa;
  sa.sa_handler = signalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  demod.start();

  auto softBitsQueue = demod.getSoftBitsQueue();
  while (!sigint) {
    // Discard items on the soft bits queue.
    // They are consumed through the publisher attached to the queue.
    auto softBits = softBitsQueue->popForRead();
    if (softBits) {
      softBitsQueue->pushRead(std::move(softBits));
      continue;
    }

    // The queue shut down; so we can shut down
    break;
  }

  demod.stop();

  return 0;
}
