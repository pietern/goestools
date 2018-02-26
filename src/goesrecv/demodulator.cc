#include "demodulator.h"

#include <pthread.h>

#include "lib/util.h"

Demodulator::Demodulator(Demodulator::Type t) {
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

  // Initialize queues
  // It is possible to attach publishers before starting the demodulator
  sourceQueue_ = std::make_shared<Queue<Samples> >(4);
  agcQueue_ = std::make_shared<Queue<Samples> >(2);
  rrcQueue_ = std::make_shared<Queue<Samples> >(2);
  costasQueue_ = std::make_shared<Queue<Samples> >(2);
  clockRecoveryQueue_ = std::make_shared<Queue<Samples> >(2);
  softBitsQueue_ = std::make_shared<Queue<std::vector<int8_t> > >(2);
}

void Demodulator::initialize(Config& config) {
  const auto& source = config.demodulator.source;
  if (source == "airspy") {
    sampleRate_ = 3000000;
    airspy_ = Airspy::open();
    airspy_->setCenterFrequency(freq_);
    airspy_->setSampleRate(sampleRate_);
    airspy_->setGain(18);
    airspy_->setSamplePublisher(std::move(config.source.samplePublisher));
  } else if (source == "rtlsdr") {
    sampleRate_ = 2400000;
    rtlsdr_ = RTLSDR::open();
    rtlsdr_->setCenterFrequency(freq_);
    rtlsdr_->setSampleRate(sampleRate_);
    rtlsdr_->setTunerGain(30);
    rtlsdr_->setSamplePublisher(std::move(config.source.samplePublisher));
  } else if (source == "nanomsg") {
    sampleRate_ = config.nanomsg.sampleRate;
    nanomsg_ = Nanomsg::open(config);
    nanomsg_->setSamplePublisher(std::move(config.source.samplePublisher));
  } else {
    assert(false);
  }

  statsPublisher_ = StatsPublisher::create(config.demodulator.statsPublisher.bind);
  if (config.demodulator.statsPublisher.sendBuffer > 0) {
    statsPublisher_->setSendBuffer(config.demodulator.statsPublisher.sendBuffer);
  }

  const auto dc = config.demodulator.decimation;
  const auto sr1 = sampleRate_;
  const auto sr2 = sampleRate_ / dc;

  agc_ = std::make_unique<AGC>();
  agc_->setSamplePublisher(std::move(config.agc.samplePublisher));

  rrc_ = std::make_unique<RRC>(dc, sr1, symbolRate_);
  rrc_->setSamplePublisher(std::move(config.rrc.samplePublisher));

  costas_ = std::make_unique<Costas>();
  costas_->setSamplePublisher(std::move(config.costas.samplePublisher));

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
        rrc_->work(agcQueue_, rrcQueue_);
        costas_->work(rrcQueue_, costasQueue_);
        clockRecovery_->work(costasQueue_, clockRecoveryQueue_);
        quantization_->work(clockRecoveryQueue_, softBitsQueue_);
        publishStats();
      }

      // Close queues to signal downstream consumers of termination
      agcQueue_->close();
      rrcQueue_->close();
      costasQueue_->close();
      clockRecoveryQueue_->close();
      softBitsQueue_->close();
    });
  pthread_setname_np(thread_.native_handle(), "demodulator");

  if (airspy_) {
    airspy_->start(sourceQueue_);
  }
  if (rtlsdr_) {
    rtlsdr_->start(sourceQueue_);
  }
  if (nanomsg_) {
    nanomsg_->start(sourceQueue_);
  }
}

void Demodulator::stop() {
  if (airspy_) {
    airspy_->stop();
  }
  if (rtlsdr_) {
    rtlsdr_->stop();
  }
  if (nanomsg_) {
    nanomsg_->stop();
  }

  thread_.join();
}
