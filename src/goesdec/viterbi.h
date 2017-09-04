#pragma once

extern "C" {
#ifdef __SSE__
#include <correct-sse.h>
#else
#include <correct.h>
#endif
}

class Viterbi {
#ifdef __SSE__
  using conv = correct_convolutional_sse;
#else
  using conv = correct_convolutional;
#endif

public:
  Viterbi() {
    // Initialize Viterbi decoder.
    // Polynomials are not referenced after creation
    // so can be referenced from the stack.
    uint16_t poly[2] = { (uint16_t)0x4f, (uint16_t)0x6d };
#ifdef __SSE__
    v_ = correct_convolutional_sse_create(2, 7, poly);
#else
    v_ = correct_convolutional_create(2, 7, poly);
#endif
  }

  ~Viterbi() {
#ifdef __SSE__
    correct_convolutional_sse_destroy(v_);
#else
    correct_convolutional_destroy(v_);
#endif
  }

  ssize_t decodeSoft(const uint8_t* encoded, size_t bits, uint8_t* msg) {
#ifdef __SSE__
    return correct_convolutional_sse_decode_soft(v_, encoded, bits, msg);
#else
    return correct_convolutional_decode_soft(v_, encoded, bits, msg);
#endif
  }

private:
  conv* v_;
};
