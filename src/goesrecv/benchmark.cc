#include <chrono>
#include <iostream>
#include <numeric>

#include "agc.h"
#include "clock_recovery.h"
#include "costas.h"
#include "quantize.h"
#include "rrc.h"
#include "types.h"

class Timer {
public:
  Timer() {
    start();
  }

  void start() {
    start_ = std::chrono::high_resolution_clock::now();
  }

  long long ns() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::nanoseconds(now - start_).count();
  }

protected:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

template <typename T>
class Benchmark {
public:
  explicit Benchmark(T& t, int blockSize)
    : t_(t),
      blockSize_(blockSize) {
    inQueue_ = std::make_shared<Queue<Samples> >(4);
    outQueue_ = std::make_shared<Queue<Samples> >(4);
  }

  void run() {
    Timer dt;

    // Run for N seconds
    std::vector<long> samples;
    while ((dt.ns() / 1000000000) < 4) {
      fillQueue();
      {
        Timer dtrun;
        t_.work(inQueue_, outQueue_);
        samples.push_back(dtrun.ns());
      }
      drainQueue();
    }

    // Log average time per block, and samples per second
    auto sum = std::accumulate(samples.begin(), samples.end(), (long long) 0);
    auto nsamples = (long long) blockSize_ * samples.size();
    std::cerr.setf(std::ios::fixed, std:: ios::floatfield);
    std::cerr.precision(3);
    std::cerr << "  Time spent in work(): "
              << sum / 1e9f
              << "s"
              << std::endl;
    std::cerr << "  Time per block:       "
              << (sum / samples.size())
              << "ns" << std::endl;
    std::cerr << "  Samples per second:   "
              << ((nsamples * 1000000000) / sum) / 1e6f
              << "M"
              << std::endl;
  }

  void fillQueue() {
    auto input = inQueue_->popForWrite();
    input->resize(blockSize_);
    inQueue_->pushWrite(std::move(input));
  }

  void drainQueue() {
    auto output = outQueue_->popForRead();
    outQueue_->pushRead(std::move(output));
  }

protected:
  T& t_;
  int blockSize_;
  std::shared_ptr<Queue<Samples> > inQueue_;
  std::shared_ptr<Queue<Samples> > outQueue_;
};

int main(int argc, char** argv) {
  std::string name;
  if (argc == 2) {
    name = std::string(argv[1]);
  }

  auto blockSize = 128 * 1024;
  if (name.empty() || name == "agc") {
    std::cerr << "AGC (block size=" << blockSize << ")" << std::endl;
    auto agc = std::make_unique<AGC>();
    auto benchmark = Benchmark<AGC>(*agc, blockSize);
    benchmark.run();
  }
  if (name.empty() || name == "costas") {
    std::cerr << "Costas (block size=" << blockSize << ")" << std::endl;
    auto costas = std::make_unique<Costas>();
    auto benchmark = Benchmark<Costas>(*costas, blockSize);
    benchmark.run();
  }
  if (name.empty() || name == "rrc") {
    std::cerr << "FIR (N=31, block size=" << blockSize << ")" << std::endl;
    // Note: filter runs WITHOUT decimation
    auto rrc = std::make_unique<RRC>(1, 3000000, 927000);
    auto benchmark = Benchmark<RRC>(*rrc, 128 * 1024);
    benchmark.run();
  }
  if (name.empty() || name == "clock") {
    std::cerr << "Clock recovery (block size=" << blockSize << ")" << std::endl;
    auto clock = std::make_unique<ClockRecovery>(3000000, 927000);
    auto benchmark = Benchmark<ClockRecovery>(*clock, 128 * 1024);
    benchmark.run();
  }
}
