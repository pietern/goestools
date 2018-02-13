#pragma once

#include <memory>
#include <string>

#include "packet_publisher.h"
#include "sample_publisher.h"
#include "soft_bit_publisher.h"

struct Config {
  struct Demodulator {
    // LRIT or HRIT
    std::string downlinkType;

    // String "airspy" or "rtlsdr"
    std::string source;
  };

  Demodulator demodulator;

  struct Source {
    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  Source source;

  struct AGC {
    std::unique_ptr<SamplePublisher> samplePublisher;
  };

  AGC agc;

  struct Costas {
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
  };

  Decoder decoder;

  static Config load(const std::string& file);
};
