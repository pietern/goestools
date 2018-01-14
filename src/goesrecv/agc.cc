#include "agc.h"

#include <cassert>
#include <cmath>

#include <simdpp/simd.h>

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

  // Input/output cursors
  std::complex<float>* fi = input->data();
  std::complex<float>* fo = output->data();

  // Process 4 samples at a time
  for (size_t i = 0; i < nsamples; i += 4) {
    // Load 4x I/Q
    simdpp::float32<4> vali;
    simdpp::float32<4> valq;
    simdpp::load_packed2(vali, valq, &fi[i]);

    // Apply gain
    simdpp::float32<4> gain = simdpp::splat(gain_);
    vali = vali * gain;
    valq = valq * gain;

    // Write to output
    simdpp::store_packed2(&fo[i], vali, valq);

    // Compute signal magnitude
    simdpp::float32<4> mag = simdpp::sqrt((vali * vali) + (valq * valq));

    // Update gain (with average magnitude of these 4 samples)
    gain_ += alpha_ * (0.5 - (simdpp::reduce_add(mag) / 4.0f));
  }

  // Return buffers
  qin->pushRead(std::move(input));
  qout->pushWrite(std::move(output));
}
