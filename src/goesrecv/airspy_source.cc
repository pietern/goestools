#include "airspy_source.h"

#include <pthread.h>

#include <cstring>
#include <iostream>

#include <util/error.h>

std::unique_ptr<Airspy> Airspy::open(uint32_t index) {
  struct airspy_device* dev = nullptr;
  auto rv = airspy_open(&dev);
  if (rv < 0) {
    std::cerr
      << "Unable to open Airspy device: "
      << airspy_error_name((enum airspy_error) rv)
      << std::endl;
    exit(1);
  }

  return std::make_unique<Airspy>(dev);
}

Airspy::Airspy(struct airspy_device* dev) : dev_(dev) {
  // Load list of supported sample rates
  sampleRates_ = loadSampleRates();

  // We produce floats so let the airspy library take care of it
  auto rv = airspy_set_sample_type(dev_, AIRSPY_SAMPLE_FLOAT32_IQ);
  ASSERT(rv == 0);
}

Airspy::~Airspy() {
  if (dev_ != nullptr) {
    airspy_close(dev_);
  }
}

std::vector<uint32_t> Airspy::loadSampleRates() {
  int rv;

  uint32_t count;
  rv = airspy_get_samplerates(dev_, &count, 0);
  ASSERT(rv == 0);

  std::vector<uint32_t> rates(count);
  rv = airspy_get_samplerates(dev_, rates.data(), rates.size());
  ASSERT(rv == 0);

  return rates;
}

void Airspy::setFrequency(uint32_t freq) {
  ASSERT(dev_ != nullptr);
  auto rv = airspy_set_freq(dev_, freq);
  ASSERT(rv >= 0);
}

void Airspy::setSampleRate(uint32_t rate) {
  ASSERT(dev_ != nullptr);
  auto rv = airspy_set_samplerate(dev_, rate);
  ASSERT(rv >= 0);
  sampleRate_ = rate;
}

uint32_t Airspy::getSampleRate() const {
  return sampleRate_;
}

void Airspy::setGain(int gain) {
  ASSERT(dev_ != nullptr);
  auto rv = airspy_set_linearity_gain(dev_, gain);
  ASSERT(rv >= 0);
}

void Airspy::setBiasTee(bool on) {
  ASSERT(dev_ != nullptr);
  auto rv = airspy_set_rf_bias(dev_, on ? 1 : 0);
  ASSERT(rv >= 0);
}

static int airspy_callback(airspy_transfer* transfer) {
  auto airspy = reinterpret_cast<Airspy*>(transfer->ctx);
  airspy->handle(transfer);
  return 0;
}

void Airspy::start(const std::shared_ptr<Queue<Samples> >& queue) {
  ASSERT(dev_ != nullptr);
  queue_ = queue;
  thread_ = std::thread([&] {
      auto rv = airspy_start_rx(dev_, &airspy_callback, this);
      ASSERT(rv == 0);
    });
#ifdef __APPLE__
  pthread_setname_np("airspy");
#else
  pthread_setname_np(thread_.native_handle(), "airspy");
#endif
}

void Airspy::stop() {
  ASSERT(dev_ != nullptr);
  auto rv = airspy_stop_rx(dev_);
  ASSERT(rv >= 0);

  // Wait for thread to terminate
  thread_.join();

  // Close queue to signal downstream
  queue_->close();

  // Clear reference to queue
  queue_.reset();
}

void Airspy::handle(const airspy_transfer* transfer) {
  auto nsamples = transfer->sample_count;
  auto out = queue_->popForWrite();
  out->resize(nsamples);
  memcpy(out->data(), transfer->samples, nsamples * sizeof(std::complex<float>));

  // Publish output if applicable
  if (samplePublisher_) {
    samplePublisher_->publish(*out);
  }

  queue_->pushWrite(std::move(out));
}
