#pragma once

#include <memory>
#include <string>

#include "packet_publisher.h"
#include "sample_publisher.h"
#include "soft_bit_publisher.h"

struct Config {
  struct StatsPublisher {
    // Addresses to bind to. This is a vector because we always add an
    // inproc:// endpoint for the in-process monitoring code.
    std::vector<std::string> bind;

    // Optional send buffer size
    size_t sendBuffer = 0;
  };

  struct Demodulator {
    // LRIT or HRIT
    std::string downlinkType;

    // String "airspy" or "rtlsdr"
    std::string source;

    // Demodulator statistics (gain, frequency correction, etc.)
    StatsPublisher statsPublisher;

    // Signal decimation (applied at FIR stage)
    int decimation = 1;
  };

  Demodulator demodulator;

  struct Airspy {
    uint32_t frequency = 0;
    uint32_t sampleRate = 0;

    // Applies to the linearity gain setting
    uint8_t gain = 18;

    // Enable/disable bias tee
    bool bias_tee = 0;

    std::unique_ptr<SamplePublisher> samplePublisher;
 };

  Airspy airspy;

  struct RTLSDR {
    uint32_t frequency = 0;
    uint32_t sampleRate = 0;

    // Applies to the tuner gain setting
    uint8_t gain = 30;

    // Enable/disable bias tee
    bool bias_tee = 0;

    // Optional device index (if you have multiple devices)
    uint32_t deviceIndex = 0;

    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  RTLSDR rtlsdr;

  struct Nanomsg {
    uint32_t sampleRate = 0;

    // Address to connect to
    std::string connect;

    // Optional receive buffer size
    size_t receiveBuffer = 0;

    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  Nanomsg nanomsg;

  struct AGC {
    // Minimum gain
    float min = 1e-6f;

    // Maximum gain
    float max = 1e+6f;

    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  AGC agc;

  struct Costas {
    // Maximum frequency deviation in Hz (defaults to 20 KHz)
    int maxDeviation = 20000;

    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  Costas costas;

  struct RRC {
    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  RRC rrc;

  struct ClockRecovery {
    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  ClockRecovery clockRecovery;

  struct Quantization {
    std::unique_ptr<SoftBitPublisher> softBitPublisher;
  };

  Quantization quantization;

  struct Decoder {
    std::unique_ptr<PacketPublisher> packetPublisher;

    // Decoder statistics (Viterbi, Reed-Solomon, etc.)
    StatsPublisher statsPublisher;
  };

  Decoder decoder;

  struct Monitor {
    // Address to send UDP statsd packets to (e.g. localhost:8125)
    std::string statsdAddress;
  };

  Monitor monitor;

  static Config load(const std::string& file);
};
