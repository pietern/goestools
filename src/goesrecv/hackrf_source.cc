#include "hackrf_source.h"

#include <pthread.h>
#include <util/error.h>

#include <cstring>
#include <iostream>

std::unique_ptr<HackRF> HackRF::open(uint32_t index) {
  struct hackrf_device* dev = nullptr;
  auto rv_init = hackrf_init();
  if (rv_init < 0) {
    std::cerr << "Unable to init hackrf" << std::endl;
    exit(1);
  }

  auto rv = hackrf_open(&dev);

  if (rv < 0) {
    std::cerr << "Unable to open HackRF device: "
              << hackrf_error_name((enum hackrf_error)rv) << std::endl;
    exit(1);
  }

  return std::make_unique<HackRF>(dev);
}

HackRF::HackRF(struct hackrf_device* dev) : dev_(dev) {
  // Load list of supported sample rates
  sampleRates_ = loadSampleRates();
}

HackRF::~HackRF() {
  if (dev_ != nullptr) {
    hackrf_close(dev_);
    hackrf_exit();
  }
}

std::vector<uint32_t> HackRF::loadSampleRates() {
  return {8000000, 10000000, 12500000, 16000000, 20000000};
}

void HackRF::setFrequency(uint32_t freq) {
  ASSERT(dev_ != nullptr);
  auto rv = hackrf_set_freq(dev_, freq);
  ASSERT(rv >= 0);
}

void HackRF::setSampleRate(uint32_t rate) {
  ASSERT(dev_ != nullptr);
  auto rv = hackrf_set_sample_rate(dev_, rate);
  ASSERT(rv >= 0);
  sampleRate_ = rate;
}

uint32_t HackRF::getSampleRate() const { return sampleRate_; }

void HackRF::setIfGain(int gain) {
  ASSERT(dev_ != nullptr);
  auto rv = hackrf_set_lna_gain(dev_, gain);
  ASSERT(rv >= 0);
}

void HackRF::setRfAmplifier(bool on) {
  ASSERT(dev_ != nullptr);
  auto rv = hackrf_set_amp_enable(dev_, on);
  ASSERT(rv >= 0);
}

void HackRF::setBbGain(int gain) {
  ASSERT(dev_ != nullptr);
  auto rv = hackrf_set_vga_gain(dev_, gain);
  ASSERT(rv >= 0);
}

void HackRF::setBiasTee(bool on) {
  ASSERT(dev_ != nullptr);
  auto rv = hackrf_set_antenna_enable(dev_, on ? 1 : 0);
  ASSERT(rv >= 0);
}

static int hackrf_callback(hackrf_transfer* transfer) {
  auto hackrf_context = reinterpret_cast<HackRF*>(transfer->rx_ctx);
  hackrf_context->handle(transfer);
  return 0;
}

void HackRF::start(const std::shared_ptr<Queue<Samples> >& queue) {
  ASSERT(dev_ != nullptr);
  queue_ = queue;
  thread_ = std::thread([&] {
    auto rv = hackrf_start_rx(dev_, &hackrf_callback, this);
    ASSERT(rv == 0);
  });
#ifdef __APPLE__
  pthread_setname_np("hackrf");
#else
  pthread_setname_np(thread_.native_handle(), "hackrf");
#endif
}

void HackRF::stop() {
  ASSERT(dev_ != nullptr);
  auto rv = hackrf_stop_rx(dev_);
  ASSERT(rv >= 0);

  // Wait for thread to terminate
  thread_.join();

  // Close queue to signal downstream
  queue_->close();

  // Clear reference to queue
  queue_.reset();
}

void HackRF::process(size_t nsamples, unsigned char* buf,
                     std::complex<float>* fo) {
  for (uint32_t i = 0; i < nsamples; i++) {
    fo[i].real((static_cast<int8_t>(buf[i * 2 + 0]) / 128.0f));
    fo[i].imag((static_cast<int8_t>(buf[i * 2 + 1]) / 128.0f));
  }
}

void HackRF::handle(const hackrf_transfer* transfer) {
  uint32_t nsamples = transfer->valid_length / 2;

  // Expect multiple of 2
  ASSERT((nsamples & 0x2) == 0);

  // Grab buffer from queue
  auto out = queue_->popForWrite();
  out->resize(nsamples);

  // Convert unsigned char to std::complex<float>
  process(nsamples, transfer->buffer, out->data());

  // Publish output if applicable
  if (samplePublisher_) {
    samplePublisher_->publish(*out);
  }

  // Return buffer to queue
  queue_->pushWrite(std::move(out));
}
