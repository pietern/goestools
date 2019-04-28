#include "soapy_source.h"

#include <pthread.h>

#include <cstring>
#include <iostream>

#include <util/error.h>
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <algorithm>

std::unique_ptr<Soapy> Soapy::open(uint32_t index) {
  size_t length;
  SoapySDRKwargs *results = SoapySDRDevice_enumerate(nullptr, &length);
  ASSERT(index < length);
  SoapySDRDevice *dev = SoapySDRDevice_make(&results[index]);
  if (dev == nullptr) {
    std::cerr
      << "Unable to open SoapySDR device: "
      << SoapySDRDevice_lastError()
      << std::endl;
    exit(1);
  }

  return std::make_unique<Soapy>(dev);
}

Soapy::Soapy(struct SoapySDRDevice* dev) : dev_(dev) {
  // Load list of supported sample rates
  sampleRates_ = loadSampleRates();
}

Soapy::~Soapy() {
  if (dev_ != nullptr) {
      SoapySDRDevice_deactivateStream(dev_, rxStream_, 0, 0); //stop streaming
      SoapySDRDevice_closeStream(dev_, rxStream_);
  }
}

std::vector<uint32_t> Soapy::loadSampleRates() {
    size_t length;
    double* dpRates = SoapySDRDevice_listSampleRates(dev_, SOAPY_SDR_RX, 0, &length);
    std::vector<double> dRates(dpRates, dpRates + length);
    std::vector<uint32_t> rates(dRates.begin(), dRates.end());

    return rates;
}

void Soapy::setFrequency(uint32_t freq) {
  ASSERT(dev_ != nullptr);
  auto rv = (int)SoapySDRDevice_setFrequency(dev_, SOAPY_SDR_RX, 0, (double)freq, NULL);
  ASSERT(rv == 0);
}

void Soapy::setSampleRate(uint32_t rate) {
  ASSERT(dev_ != nullptr);
  auto rv = (int)SoapySDRDevice_setSampleRate(dev_, SOAPY_SDR_RX, 0, (double)rate);
  ASSERT(rv == 0);
  sampleRate_ = rate;
}

uint32_t Soapy::getSampleRate() const {
  return sampleRate_;
}

void Soapy::setGain(int gain) {
  ASSERT(dev_ != nullptr);
  auto rv = (int)SoapySDRDevice_setGain(dev_, SOAPY_SDR_RX, 0, (double)gain);
  ASSERT(rv == 0);
}

void Soapy::start(const std::shared_ptr<Queue<Samples> >& queue) {
  ASSERT(dev_ != nullptr);
  queue_ = queue;
  thread_ = std::thread([&] {
      std::cout << "Setting up stream" << std::endl;
      if (SoapySDRDevice_setupStream(dev_, &rxStream_, SOAPY_SDR_RX, SOAPY_SDR_CF32, nullptr, 0, nullptr) != 0)
      {
          std::cerr
                  << "Failed to set up stream: "
                  << SoapySDRDevice_lastError()
                  << std::endl;
          exit(1);
      }
      std::cout << "Activating stream" << std::endl;
      if (SoapySDRDevice_activateStream(dev_, rxStream_, 0, 0, 0) != 0) {
          std::cerr
                  << "Failed to activate stream"
                  << std::endl;
          exit(1);
      }
      std::cout << "Starting read loop" << std::endl;
      std::complex<double> buff[1024];
      while (!queue_->closed())
      {
          void *buffs[] = {buff}; //array of buffers
          int flags; //flags set by receive operation
          long long timeNs; //timestamp for receive buffer
          int ret = SoapySDRDevice_readStream(dev_, rxStream_, buffs, 1024, &flags, &timeNs, 100000);

          if (ret < 0) {
              std::cerr
                      << "Failed to read stream: "
                      << ret
                      << std::endl;
              exit(1);
          }

          this->handle(ret, buff);
      }
    });
  // TODO: nothing works if the handle method doesn't get called on the main thread, but join blocks the exec path
    thread_.join();
#ifdef __APPLE__
  pthread_setname_np("soapy");
#else
  pthread_setname_np(thread_.native_handle(), "soapy");
#endif
}

void Soapy::stop() {
    ASSERT(dev_ != nullptr);

    // Close queue to signal downstream
    queue_->close();

    // Clear reference to queue
    queue_.reset();
}

void Soapy::handle(const int nsamples, std::complex<double>* buff) {
  auto out = queue_->popForWrite();
  out->resize(nsamples);
  std::complex<float> fBuff[1024];
  std::transform(buff, buff + nsamples, fBuff, [](std::complex<double> x) -> std::complex<float>{
      return std::complex<float>(x);
  });
  memcpy(out->data(), fBuff, nsamples * sizeof(std::complex<float>));

  // Publish output if applicable
  if (samplePublisher_) {
    samplePublisher_->publish(*out);
  }

  queue_->pushWrite(std::move(out));
}
