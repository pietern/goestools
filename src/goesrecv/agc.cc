#include "agc.h"

#include <cassert>
#include <cmath>

#include <arm_neon.h>

AGC::AGC() {
  alpha_ = 1e-4f;
  gain_ = 1e0f;
  acc_ = 1e0f;
}

void AGC::work(
    const std::shared_ptr<Queue<Samples> >& qin,
    const std::shared_ptr<Queue<Samples> >& qout) {
  auto input = qin->popForRead();
  auto output = qout->popForWrite();
  auto nsamples = input->size();
  output->resize(nsamples);

  // Below should work generically with libsimdpp, but results in
  // a SIGBUS on a Raspberry Pi. I think simdpp::load_splat is the
  // culprit. Further investigation needed.
  //
  // // Input/output cursors
  // std::complex<float>* fi = input->data();
  // std::complex<float>* fo = output->data();
  //
  // // Process 4 samples at a time
  // for (size_t i = 0; i < nsamples; i += 4) {
  //   // Load 4x I/Q
  //   simdpp::float32<4> vali;
  //   simdpp::float32<4> valq;
  //   simdpp::load_packed2(vali, valq, &fi[i]);
  //
  //   // Apply gain
  //   simdpp::float32<4> gain = simdpp::load_splat(&gain_);
  //   vali = vali * gain;
  //   valq = valq * gain;
  //
  //   // Write to output
  //   simdpp::store_packed2(&fo[i], vali, valq);
  //
  //   // Compute signal magnitude
  //   simdpp::float32<4> mag = simdpp::sqrt((vali * vali) + (valq * valq));
  //
  //   // Update gain (with average magnitude of these 4 samples)
  //   gain_ += alpha_ * (0.5 - (simdpp::reduce_add(mag) / 4.0f));
  // }

  float* fi = (float*) input->data();
  float* fo = (float*) output->data();

  // Process 4 samples at a time.
  for (size_t i = 0; i < nsamples; i += 4) {
    float32x4x2_t f = vld2q_f32(&fi[2*i]);

    // Apply gain.
    float32x4_t gain = vld1q_dup_f32(&gain_);
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
    gain_ += alpha_ * (0.5 - sqrtf(x2[0]));
  }

  // Return buffers
  qin->pushRead(std::move(input));
  qout->pushWrite(std::move(output));
}
