#include "costas.h"

#include <cmath>

#include <util/error.h>

#ifdef __ARM_NEON
#include "./neon/neon_mathfun.h"
#endif

#define M_2PI (2 * M_PI)

Costas::Costas() {
  float damp = sqrtf(2.0f)/2.0f;
  float bw = 0.005f;
  phase_ = 0.0f;
  freq_ = 0.0f;
  alpha_ = (4 * damp * bw) / (1.0 + 2.0 * damp * bw + bw * bw);
  beta_ = (4 * bw * bw) / (1.0 + 2.0 * damp * bw + bw * bw);
  maxDeviation_ = M_2PI;
}

#ifdef __ARM_NEON

void Costas::work(
    size_t nsamples,
    std::complex<float>* fi,
    std::complex<float>* fo) {
  // Needed for clipping in loop body
  float pos1_ = +1.0f;
  float neg1_ = -1.0f;
  float half_ = 0.5f;
  float32x4_t pos1 = vld1q_dup_f32(&pos1_);
  float32x4_t neg1 = vld1q_dup_f32(&neg1_);
  float32x4_t half = vld1q_dup_f32(&half_);

  for (size_t i = 0; i < nsamples; i += 4) {
    float32x4_t phase = {
      -(phase_ + 0 * freq_),
      -(phase_ + 1 * freq_),
      -(phase_ + 2 * freq_),
      -(phase_ + 3 * freq_),
    };

    // Compute sin/cos for phase offset
    float32x4_t sin;
    float32x4_t cos;
    sincos_ps(phase, &sin, &cos);

    // Load 4 samples into 2 registers (in-phase and quadrature)
    float32x4x2_t f = vld2q_f32((const float32_t*) &fi[i]);

    // Complex multiplication
    // (a + ib) * (c + id) expands to:
    // Real: (ac - bd)
    // Imaginary: (ad + cb)i
    // Here, a is f.val[0], b is f.val[1], c is cos, and d is sin.
    float32x4_t ac = vmulq_f32(f.val[0], cos);
    float32x4_t bd = vmulq_f32(f.val[1], sin);
    float32x4_t ad = vmulq_f32(f.val[0], sin);
    float32x4_t bc = vmulq_f32(f.val[1], cos);
    f.val[0] = vsubq_f32(ac, bd);
    f.val[1] = vaddq_f32(ad, bc);

    // Write 4 samples back to memory
    vst2q_f32((float32_t*) &fo[i], f);

    // Phase detector is executed for all samples,
    // Clip resulting value to [-1.0f, 1.0f]
    // Total error is average of 4 errors.
    float32x4_t err = vmulq_f32(f.val[0], f.val[1]);
    float32x4_t err_pos1 = vabsq_f32(vaddq_f32(err, pos1));
    float32x4_t err_neg1 = vabsq_f32(vaddq_f32(err, neg1));
    err = vmulq_f32(half, vsubq_f32(err_pos1, err_neg1));
    float terr = (err[0] + err[1] + err[2] + err[3]) / 4.0f;

    // Update frequency and phase
    freq_ += beta_ * terr;
    phase_ += alpha_ * terr + freq_;

    // Clamp frequency
    freq_ = (0.5f * (fabsf(freq_ + maxDeviation_) -
                     fabsf(freq_ - maxDeviation_)));

    // Wrap phase if needed
    if (phase_ > M_2PI || phase_ < -M_2PI) {
      float frac = phase_ * (1.0 / M_2PI);
      phase_ = (frac - (float)((int)frac)) * M_2PI;
    }
  }
}

#else

void Costas::work(
    size_t nsamples,
    std::complex<float>* fi,
    std::complex<float>* fo) {
  for (size_t i = 0; i < nsamples; i += 4) {
    float phase[4] = {
      -(phase_ + 0 * freq_),
      -(phase_ + 1 * freq_),
      -(phase_ + 2 * freq_),
      -(phase_ + 3 * freq_),
    };

    // Compute sin/cos for phase offset
    std::complex<float> sincos[4];
    for (size_t j = 0; j < 4; j++) {
      sincos[j].real(cosf(phase[j]));
      sincos[j].imag(sinf(phase[j]));
    }

    // Complex multiplication
    for (size_t j = 0; j < 4; j++) {
      fo[i + j] = fi[i + j] * sincos[j];
    }

    // Phase detector is executed for all samples,
    // Clip resulting value to [-1.0f, 1.0f].
    // Total error is average of 4 errors.
    float terr = 0.0f;
    for (size_t j = 0; j < 4; j++) {
      float err = fo[i + j].real() * fo[i + j].imag();
      terr += (0.5f * (fabsf(err + 1.0f) - fabsf(err - 1.0f))) / 4.0f;
    }

    // Update frequency and phase
    freq_ += beta_ * terr;
    phase_ += alpha_ * terr + freq_;

    // Clamp frequency
    freq_ = (0.5f * (fabsf(freq_ + maxDeviation_) -
                     fabsf(freq_ - maxDeviation_)));

    // Wrap phase if needed
    if (phase_ > M_2PI || phase_ < -M_2PI) {
      float frac = phase_ * (1.0 / M_2PI);
      phase_ = (frac - (float)((int)frac)) * M_2PI;
    }
  }
}

#endif

void Costas::work(
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

  // Assume multiple of 4 number of samples
  ASSERT((nsamples % 4) == 0);

  // Do actual work
  std::complex<float>* fi = input->data();
  std::complex<float>* fo = output->data();
  work(nsamples, fi, fo);

  // Return input buffer
  qin->pushRead(std::move(input));

  // Publish output if applicable
  if (samplePublisher_) {
    samplePublisher_->publish(*output);
  }

  // Return output buffer
  qout->pushWrite(std::move(output));
}
