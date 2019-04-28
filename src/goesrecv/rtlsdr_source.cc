#include "rtlsdr_source.h"

#include <pthread.h>

#include <climits>
#include <cmath>
#include <iostream>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include <util/error.h>

std::unique_ptr<RTLSDR> RTLSDR::open(uint32_t index) {
  rtlsdr_dev_t* dev = nullptr;
  auto rv = rtlsdr_open(&dev, index);
  if (rv < 0) {
    std::cerr << "Unable to open RTLSDR device..." << std::endl;
    exit(1);
  }

  return std::make_unique<RTLSDR>(dev);
}

RTLSDR::RTLSDR(rtlsdr_dev_t* dev) : dev_(dev) {
  int rv;

  // First get the number of gain settings
  rv = rtlsdr_get_tuner_gains(dev_, nullptr);
  ASSERT(rv > 0);

  // Now fill our vector with valid gain settings
  tunerGains_.resize(rv);
  rv = rtlsdr_get_tuner_gains(dev_, tunerGains_.data());
  ASSERT(rv >= 0);

  // Configure manual gain mode
  rv = rtlsdr_set_tuner_gain_mode(dev_, 1);
  ASSERT(rv >= 0);

  // Disable internal AGC
  rv = rtlsdr_set_agc_mode(dev_, 0);
  ASSERT(rv >= 0);

  // Disable direct sampling
  // This is not supported by all dongles, ignore return value
  rtlsdr_set_direct_sampling(dev_, 0);

  // Disable offset tuning
  rv = rtlsdr_set_offset_tuning(dev_, 0);
  ASSERT(rv == -2 || rv >= 0);

  // Tune to 100MHz to initialize.
  //
  // Even though the device will only be tuned here for a fraction of
  // a second, it makes the difference between not being able to tune
  // 99% of the time (and getting a garbage signal) and always being
  // able to tune, on a particular unit with R820T chip.
  //
  // Since it doesn't hurt, we can just try, and hopefully increase
  // our chances of being able to tune across the board.
  //
  rv = rtlsdr_set_center_freq(dev_, 100000000);
  ASSERT(rv >= 0);
}

RTLSDR::~RTLSDR() {
  if (dev_ != nullptr) {
    rtlsdr_close(dev_);
  }
}

void RTLSDR::setFrequency(uint32_t freq) {
  ASSERT(dev_ != nullptr);
  auto rv = rtlsdr_set_center_freq(dev_, freq);
  ASSERT(rv >= 0);
}

void RTLSDR::setSampleRate(uint32_t rate) {
  ASSERT(dev_ != nullptr);
  auto rv = rtlsdr_set_sample_rate(dev_, rate);
  ASSERT(rv >= 0);
}

uint32_t RTLSDR::getSampleRate() const {
  ASSERT(dev_ != nullptr);
  return rtlsdr_get_sample_rate(dev_);
}

void RTLSDR::setTunerGain(int db) {
  int rv;

  // Find closest valid gain setting
  float resultDist = INT_MAX;
  int resultGain = 0;
  for (const auto& gain : tunerGains_) {
    float dist = fabsf((gain / 10.0f) - db);
    if (dist < resultDist) {
      resultDist = dist;
      resultGain = gain;
    }
  }

  rv = rtlsdr_set_tuner_gain(dev_, resultGain);
  ASSERT(rv >= 0);
}

void RTLSDR::setBiasTee(bool on) {
  ASSERT(dev_ != nullptr);
#ifdef RTLSDR_HAS_BIAS_TEE
  auto rv = rtlsdr_set_bias_tee(dev_, on ? 1 : 0);
  ASSERT(rv >= 0);
#else
  if (on) {
    throw std::invalid_argument(
      "Need librtlsdr >= 0.5.4 to use bias tee.");
  }
#endif
}

static void rtlsdr_callback(unsigned char* buf, uint32_t len, void* ptr) {
  RTLSDR* rtlsdr = reinterpret_cast<RTLSDR*>(ptr);
  rtlsdr->handle(buf, len);
}

void RTLSDR::start(const std::shared_ptr<Queue<Samples> >& queue) {
  ASSERT(dev_ != nullptr);
  rtlsdr_reset_buffer(dev_);
  queue_ = queue;
  thread_ = std::thread([&] {
      rtlsdr_read_async(dev_, rtlsdr_callback, this, 0, 0);
    });
#ifdef __APPLE__
  pthread_setname_np("rtlsdr");
#else
  pthread_setname_np(thread_.native_handle(), "rtlsdr");
#endif
}

void RTLSDR::stop() {
  ASSERT(dev_ != nullptr);
  auto rv = rtlsdr_cancel_async(dev_);
  ASSERT(rv >= 0);

  // Wait for thread to terminate
  thread_.join();

  // Close queue to signal downstream
  queue_->close();

  // Clear reference to queue
  queue_.reset();
}

#ifdef __ARM_NEON

void RTLSDR::process(
    size_t nsamples,
    unsigned char* buf,
    std::complex<float>* fo) {
  // Number to subtract from samples for normalization
  // See http://cgit.osmocom.org/gr-osmosdr/tree/lib/rtl/rtl_source_c.cc#n176
  const float32_t norm_ = 127.4f / 128.0f;
  float32x4_t norm = vld1q_dup_f32(&norm_);

  // Iterate over samples in blocks of 4 (each sample has I and Q)
  for (uint32_t i = 0; i < (nsamples / 4); i++) {
    uint32x4_t ui;
    ui[0] = buf[i * 8 + 0];
    ui[1] = buf[i * 8 + 2];
    ui[2] = buf[i * 8 + 4];
    ui[3] = buf[i * 8 + 6];

    uint32x4_t uq;
    uq[0] = buf[i * 8 + 1];
    uq[1] = buf[i * 8 + 3];
    uq[2] = buf[i * 8 + 5];
    uq[3] = buf[i * 8 + 7];

    // Convert to float32x4 and divide by 2^7 (128)
    float32x4_t fi = vcvtq_n_f32_u32(ui, 7);
    float32x4_t fq = vcvtq_n_f32_u32(uq, 7);

    // Subtract to normalize to [-1.0, 1.0]
    float32x4x2_t fiq;
    fiq.val[0] = vsubq_f32(fi, norm);
    fiq.val[1] = vsubq_f32(fq, norm);

    // Store in output
    vst2q_f32((float*) (&fo[i * 4]), fiq);
  }
}

#else

void RTLSDR::process(
    size_t nsamples,
    unsigned char* buf,
    std::complex<float>* fo) {
  // Number to subtract from samples for normalization
  // See http://cgit.osmocom.org/gr-osmosdr/tree/lib/rtl/rtl_source_c.cc#n176
  const float norm = 127.4f / 128.0f;
  for (uint32_t i = 0; i < nsamples; i++) {
    fo[i].real((buf[i*2+0] / 128.0f) - norm);
    fo[i].imag((buf[i*2+1] / 128.0f) - norm);
  }
}

#endif

void RTLSDR::handle(unsigned char* buf, uint32_t len) {
  uint32_t nsamples = len / 2;

  // Expect multiple of 4
  ASSERT((nsamples & 0x3) == 0);

  // Grab buffer from queue
  auto out = queue_->popForWrite();
  out->resize(nsamples);

  // Convert unsigned char to std::complex<float>
  process(nsamples, buf, out->data());

  // Publish output if applicable
  if (samplePublisher_) {
    samplePublisher_->publish(*out);
  }

  // Return buffer to queue
  queue_->pushWrite(std::move(out));
}
