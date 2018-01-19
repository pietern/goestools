#include "demodulator.h"

#include <pthread.h>

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

bool Demodulator::configureSource(const std::string& source) {
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

void Demodulator::start() {
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
}

void Demodulator::stop() {
  if (airspy_) {
    airspy_->stop();
  }
  if (rtlsdr_) {
    rtlsdr_->stop();
  }

  thread_.join();
}
