#include "monitor.h"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <json11.hpp>
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#include <pthread.h>

namespace {

int connect(const std::string& endpoint) {
  auto fd = nn_socket(AF_SP, NN_SUB);
  if (fd < 0) {
    std::stringstream ss;
    ss << "nn_socket: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }

  auto rv = nn_connect(fd, endpoint.c_str());
  if (rv < 0) {
    nn_close(fd);
    std::stringstream ss;
    ss << "nn_connect: " << nn_strerror(nn_errno());
    ss << " (" << endpoint << ")";
    throw std::runtime_error(ss.str());
  }

  rv = nn_setsockopt(fd, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  if (rv < 0) {
    nn_close(fd);
    std::stringstream ss;
    ss << "nn_setsockopt: " << nn_strerror(nn_errno());
    ss << " (" << endpoint << ")";
    throw std::runtime_error(ss.str());
  }

  return fd;
}

template <typename T>
T sum(const std::vector<T>& vs) {
  T r = 0;
  for (const auto& v : vs) {
    r += v;
  }
  return r;
}

template <typename T>
T avg(const std::vector<T>& vs) {
  if (vs.size() == 0) {
    return 0;
  }
  return sum(vs) / vs.size();
}

} // namespace

Monitor::Monitor(bool verbose, std::chrono::milliseconds interval)
    : verbose_(verbose),
      interval_(interval),
      stop_(false) {
}

Monitor::~Monitor() {
}

void Monitor::initialize(Config& config) {
  const std::string inprocPrefix = "inproc://";

  // Connect to demodulator stats publisher
  std::string demodulatorEndpoint;
  for (const auto& endpoint : config.demodulator.statsPublisher.bind) {
    if (endpoint.compare(0, inprocPrefix.size(), inprocPrefix) == 0) {
      demodulatorEndpoint = endpoint;
      break;
    }
  }
  if (demodulatorEndpoint.size() == 0) {
    throw std::runtime_error("no demodulator stats endpoint");
  }
  demodulatorFd_ = connect(demodulatorEndpoint);

  // Connect to decoder stats publisher
  std::string decoderEndpoint;
  for (const auto& endpoint : config.decoder.statsPublisher.bind) {
    if (endpoint.compare(0, inprocPrefix.size(), inprocPrefix) == 0) {
      decoderEndpoint = endpoint;
      break;
    }
  }
  if (decoderEndpoint.size() == 0) {
    throw std::runtime_error("no decoder stats endpoint");
  }
  decoderFd_ = connect(decoderEndpoint);

  // Create statsd socket if one is configured
  const auto& statsdAddress = config.monitor.statsdAddress;
  if (!statsdAddress.empty()) {
    statsd_ = std::make_unique<DatagramSocket>(statsdAddress);
  }
}

void Monitor::loop() {
  struct nn_pollfd pfd[2];
  pfd[0].fd = demodulatorFd_;
  pfd[0].events = NN_POLLIN;
  pfd[1].fd = decoderFd_;
  pfd[1].events = NN_POLLIN;

  auto start = std::chrono::steady_clock::now();
  while (!stop_) {
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
    if (delta >= interval_) {
      Stats tmp;
      std::swap(tmp, stats_);
      if (verbose_) {
        print(tmp);
      }
      start += interval_;
      continue;
    }

    const auto timeout = (interval_ - delta).count() + 1;
    auto rv = nn_poll(pfd, 2, timeout);
    if (rv == 0) {
      continue;
    }

    for (auto i = 0; i < 2; i++) {
      if (!(pfd[i].revents & NN_POLLIN)) {
        continue;
      }

      char* buf = nullptr;
      rv = nn_recv(pfd[i].fd, &buf, NN_MSG, 0);
      if (rv < 0) {
        // Either fd is closed; terminate
        return;
      }

      process(std::string(buf, rv));

      // Processed message; free nanomsg buffer
      nn_freemsg(buf);
    }
  }
}

void Monitor::process(const std::string& json) {
  std::string error;
  auto data = json11::Json::parse(json, error);
  if (!error.empty()) {
    throw new std::runtime_error(error);
  }

  // Accumulate statsd compatible payload
  std::stringstream statsd;

  for (const auto& it : data.object_items()) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "gain") {
      stats_.gain.push_back(value.number_value());
      statsd << key << ":" << value.number_value() << "|g" << std::endl;
      continue;
    }

    if (key == "frequency") {
      stats_.frequency.push_back(value.number_value());
      statsd << key << ":" << value.number_value() << "|g" << std::endl;
      continue;
    }

    if (key == "omega") {
      stats_.omega.push_back(value.number_value());
      statsd << key << ":" << value.number_value() << "|g" << std::endl;
      continue;
    }

    if (key == "viterbi_errors") {
      stats_.viterbiErrors.push_back(value.int_value());
      statsd << key << ":" << value.int_value() << "|h" << std::endl;
      continue;
    }

    if (key == "reed_solomon_errors") {
      const auto& v = value.int_value();
      if (v >= 0) {
        stats_.reedSolomonErrors.push_back(v);
        statsd << key << ":" << v << "|h" << std::endl;
      }
      continue;
    }

    if (key == "ok") {
      const auto& ok = value.int_value();
      if (ok != 0) {
        stats_.totalOK++;
        statsd << "packets_ok:1|c" << std::endl;
      } else {
        stats_.totalDropped++;
        statsd << "packets_dropped:1|c" << std::endl;
      }
      continue;
    }
  }

  if (statsd_) {
    statsd_->send(statsd.str());
  }
}

void Monitor::print(const Stats& stats) {
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(interval_).count();

  // Upper bound of 60 packets per second (HRIT)
  const auto packetWidth = ceilf(log10f(sec * 60));

  // std::put_time is not available in GCC < 5
  std::time_t t = std::time(nullptr);
  std::array<char, 128> tsbuf;
  strftime(tsbuf.data(), tsbuf.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));

  std::stringstream ss;
  ss << std::fixed;
  ss << std::string(tsbuf.data())
     << " [monitor] ";
  ss << "gain: "
     << std::setprecision(2) << std::setw(5)
     << avg(stats.gain) << ", ";
  ss << "freq: "
     << std::setprecision(1) << std::setw(7)
     << avg(stats.frequency) << ", ";
  ss << "omega: "
     << std::setprecision(3) << std::setw(5)
     << avg(stats.omega) << ", ";
  ss << "vit(avg): "
     << std::setw(4)
     << avg(stats.viterbiErrors) << ", ";
  ss << "rs(sum): "
     << std::setw(4)
     << sum(stats.reedSolomonErrors) << ", ";
  ss << "packets: "
     << std::setw(packetWidth)
     << stats.totalOK << ", ";
  ss << "drops: "
     << std::setw(packetWidth)
     << stats.totalDropped;
  std::cout << ss.str() << std::endl;
}

void Monitor::start() {
  thread_ = std::thread(&Monitor::loop, this);
  pthread_setname_np(thread_.native_handle(), "monitor");
}

void Monitor::stop() {
  nn_close(demodulatorFd_);
  nn_close(decoderFd_);
  stop_ = true;

  // Wait for thread to terminate
  thread_.join();
}
