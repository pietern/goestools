#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "config.h"
#include "datagram_socket.h"

class Monitor {
public:
  explicit Monitor(bool verbose, std::chrono::milliseconds interval);
  ~Monitor();

  void initialize(Config& config);

  void start();
  void stop();

protected:
  struct Stats {
    // Demodulator stats
    std::vector<float> gain;
    std::vector<float> frequency;
    std::vector<float> omega;

    // Decoder stats
    std::vector<int> viterbiErrors;
    std::vector<int> reedSolomonErrors;
    int totalOK = 0;
    int totalDropped = 0;
  };

  Stats stats_;

  void loop();
  void process(const std::string& json);
  void print(const Stats& stats);

  const bool verbose_;
  const std::chrono::milliseconds interval_;

  int demodulatorFd_;
  int decoderFd_;
  std::unique_ptr<DatagramSocket> statsd_;

  std::atomic<bool> stop_;
  std::thread thread_;
};
