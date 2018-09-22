#include "timer.h"

Timer::Timer() : start_(std::chrono::high_resolution_clock::now()) {
}

std::chrono::duration<double> Timer::elapsed() const {
  return std::chrono::high_resolution_clock::now() - start_;
}
