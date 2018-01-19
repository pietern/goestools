#include "airspy_source.h"

#include <pthread.h>

#include <cassert>
#include <cstring>
#include <iostream>

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
  assert(rv == 0);
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
  assert(rv == 0);

  std::vector<uint32_t> rates(count);
  rv = airspy_get_samplerates(dev_, rates.data(), rates.size());
  assert(rv == 0);

  return rates;
}

void Airspy::setCenterFrequency(uint32_t freq) {
  assert(dev_ != nullptr);
  auto rv = airspy_set_freq(dev_, freq);
  assert(rv >= 0);
}

void Airspy::setSampleRate(uint32_t rate) {
  assert(dev_ != nullptr);
  auto rv = airspy_set_samplerate(dev_, rate);
  assert(rv >= 0);
}

void Airspy::setGain(int gain) {
  assert(dev_ != nullptr);
  auto rv = airspy_set_linearity_gain(dev_, gain);
  assert(rv >= 0);
}

void Airspy::setPublisher(std::unique_ptr<Publisher> publisher) {
  publisher_ = std::move(publisher);
}

static int airspy_callback(airspy_transfer* transfer) {
  auto airspy = reinterpret_cast<Airspy*>(transfer->ctx);
  airspy->handle(transfer);
  return 0;
}

void Airspy::start(const std::shared_ptr<Queue<Samples> >& queue) {
  assert(dev_ != nullptr);
  queue_ = queue;
  thread_ = std::thread([&] {
      auto rv = airspy_start_rx(dev_, &airspy_callback, this);
      assert(rv == 0);
    });
  pthread_setname_np(thread_.native_handle(), "airspy");
}

void Airspy::stop() {
  assert(dev_ != nullptr);
  auto rv = airspy_stop_rx(dev_);
  assert(rv >= 0);

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
  if (publisher_) {
    publisher_->publish(*out);
  }

  queue_->pushWrite(std::move(out));
}
