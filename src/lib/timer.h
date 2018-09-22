#pragma once

#include <chrono>

class Timer {
public:
  explicit Timer();

  std::chrono::duration<double> elapsed() const;

protected:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};
