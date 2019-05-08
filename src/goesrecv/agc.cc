#include "agc.h"

#include <cmath>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

AGC::AGC() {
  min_ = 1e-6f;
  max_ = 1e+6f;
  alpha_ = 1e-4f;
  gain_ = 1e0f;
}

#ifdef __ARM_NEON

void AGC::work(
    size_t nsamples,
    std::complex<float>* ci,
    std::complex<float>* co) {
  float* fi = (float*) ci;
  float* fo = (float*) co;

  float32x4_t min = vld1q_dup_f32(&min_);
  float32x4_t max = vld1q_dup_f32(&max_);
  float32x4_t gain = vld1q_dup_f32(&gain_);

  // Process 4 samples at a time.
  for (size_t i = 0; i < nsamples; i += 4) {
    float32x4x2_t f = vld2q_f32(&fi[2*i]);

    // Apply gain.
    f.val[0] = vmulq_f32(f.val[0], gain);
    f.val[1] = vmulq_f32(f.val[1], gain);
    vst2q_f32(&fo[2*i], f);

    // Compute signal magnitude.
    float32x4_t x2 =
      vaddq_f32(
        vmulq_f32(f.val[0], f.val[0]),
        vmulq_f32(f.val[1], f.val[1]));

    // Update gain.
    // Use only the first sample and ignore the others.
    float32x4_t delta = vdupq_n_f32(alpha_ * (0.5 - sqrtf(x2[0])));
    gain = vaddq_f32(gain, delta);
    gain = vmaxq_f32(gain, min);
    gain = vminq_f32(gain, max);
  }

  // Write back to instance variable
  gain_ = gain[0];
}

#else

void AGC::work(
    size_t nsamples,
    std::complex<float>* ci,
    std::complex<float>* co) {
  // Process 4 samples at a time.
  for (size_t i = 0; i < nsamples; i += 4) {
    // Apply gain
    co[i+0] = ci[i+0] * gain_;
    co[i+1] = ci[i+1] * gain_;
    co[i+2] = ci[i+2] * gain_;
    co[i+3] = ci[i+3] * gain_;

    // Update gain.
    // Use only the first sample and ignore the others.
    gain_ += alpha_ * (0.5 - abs(co[i]));
    gain_ = std::max(gain_, min_);
    gain_ = std::min(gain_, max_);
  }
}

#endif

void AGC::work(
    const std::shared_ptr<Queue<Samples> >& qin,
    const std::shared_ptr<Queue<Samples> >& qout) {
  auto input = qin->popForRead();
  if (!input) {
    qout->close();
    return;
  }

  auto output = qout->popForWrite();
  auto nsamples = input->size();
  output->resize(nsamples);

  // Do actual work
  auto ci = input->data();
  auto co = output->data();
  work(nsamples, ci, co);

  // Return input buffer
  qin->pushRead(std::move(input));

  // Publish output if applicable
  if (samplePublisher_) {
    samplePublisher_->publish(*output);
  }

  // Return output buffer
  qout->pushWrite(std::move(output));
}
