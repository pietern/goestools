#include "demodulator.h"

#include <pthread.h>

#include <util/error.h>
#include <util/time.h>

using namespace util;

Demodulator::Demodulator(Demodulator::Type t) {
  switch (t) {
  case LRIT:
    symbolRate_ = 293883;
    break;
  case HRIT:
    symbolRate_ = 927000;
    break;
  default:
    ASSERT(false);
  }

  // Sample rate depends on source
  sampleRate_ = 0;

  // Initialize queues
  // It is possible to attach publishers before starting the demodulator
  sourceQueue_ = std::make_shared<Queue<Samples> >(4);
  agcQueue_ = std::make_shared<Queue<Samples> >(2);
  costasQueue_ = std::make_shared<Queue<Samples> >(2);
  rrcQueue_ = std::make_shared<Queue<Samples> >(2);
  clockRecoveryQueue_ = std::make_shared<Queue<Samples> >(2);
  softBitsQueue_ = std::make_shared<Queue<std::vector<int8_t> > >(2);
}

void Demodulator::initialize(Config& config) {
  source_ = Source::build(config.demodulator.source, config);
  sampleRate_ = source_->getSampleRate();

  statsPublisher_ = StatsPublisher::create(config.demodulator.statsPublisher.bind);
  if (config.demodulator.statsPublisher.sendBuffer > 0) {
    statsPublisher_->setSendBuffer(config.demodulator.statsPublisher.sendBuffer);
  }

  const auto dc = config.demodulator.decimation;
  const auto sr1 = sampleRate_;
  const auto sr2 = sampleRate_ / dc;

  agc_ = std::make_unique<AGC>();
  agc_->setMin(config.agc.min);
  agc_->setMax(config.agc.max);
  agc_->setSamplePublisher(std::move(config.agc.samplePublisher));

  // Maximum frequency deviation in radians per sample (given in Hz)
  const auto maxDeviation =
    (config.costas.maxDeviation * 2 * M_PI) / sampleRate_;
  costas_ = std::make_unique<Costas>();
  costas_->setMaxDeviation(maxDeviation);
  costas_->setSamplePublisher(std::move(config.costas.samplePublisher));

  rrc_ = std::make_unique<RRC>(dc, sr1, symbolRate_);
  rrc_->setSamplePublisher(std::move(config.rrc.samplePublisher));

  clockRecovery_ = std::make_unique<ClockRecovery>(sr2, symbolRate_);
  clockRecovery_->setSamplePublisher(std::move(config.clockRecovery.samplePublisher));

  quantization_ = std::make_unique<Quantize>();
  quantization_->setSoftBitPublisher(std::move(config.quantization.softBitPublisher));
}

void Demodulator::publishStats() {
  if (!statsPublisher_) {
    return;
  }

  const auto timestamp = stringTime();
  const auto gain = agc_->getGain();
  const auto frequency = (sampleRate_ * costas_->getFrequency()) / (2 * M_PI);
  const auto omega = clockRecovery_->getOmega();

  std::stringstream ss;
  ss.precision(10);
  ss << std::scientific;
  ss << "{";
  ss << "\"timestamp\": \"" << timestamp << "\",";
  ss << "\"gain\": " << gain << ",";
  ss << "\"frequency\": " << frequency << ",";
  ss << "\"omega\": " << omega;
  ss << "}\n";
  statsPublisher_->publish(ss.str());
}

void Demodulator::start() {
  thread_ = std::thread([&] {
      while (!sourceQueue_->closed()) {
        agc_->work(sourceQueue_, agcQueue_);
        costas_->work(agcQueue_, costasQueue_);
        rrc_->work(costasQueue_, rrcQueue_);
        clockRecovery_->work(rrcQueue_, clockRecoveryQueue_);
        quantization_->work(clockRecoveryQueue_, softBitsQueue_);
        publishStats();
      }

      // Close queues to signal downstream consumers of termination
      agcQueue_->close();
      costasQueue_->close();
      rrcQueue_->close();
      clockRecoveryQueue_->close();
      softBitsQueue_->close();
    });
#ifdef __APPLE__
  pthread_setname_np("demodulator");
#else
  pthread_setname_np(thread_.native_handle(), "demodulator");
#endif
  source_->start(sourceQueue_);
}

void Demodulator::stop() {
  source_->stop();
  thread_.join();
}
