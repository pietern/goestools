#include "config.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <toml/toml.h>

namespace {

void throwInvalidKey(const std::string& key) {
  std::stringstream ss;
  ss << "Invalid configuration key: " << key;
  throw std::invalid_argument(ss.str());
}

std::unique_ptr<SamplePublisher> createSamplePublisher(const toml::Value& v) {
  auto bind = v.find("bind");
  if (!bind) {
    throw std::invalid_argument("Expected publisher section to have \"bind\" key");
  }

  // Only need bind value to create publisher
  auto p = SamplePublisher::create(bind->as<std::string>());

  // Optional send buffer size
  auto sendBuffer = v.find("send_buffer");
  if (sendBuffer) {
    p->setSendBuffer(sendBuffer->as<int>());
  }

  return p;
}

std::unique_ptr<SoftBitPublisher> createSoftBitPublisher(const toml::Value& v) {
  auto bind = v.find("bind");
  if (!bind) {
    throw std::invalid_argument("Expected publisher section to have \"bind\" key");
  }

  // Only need bind value to create publisher
  auto p = SoftBitPublisher::create(bind->as<std::string>());

  // Optional send buffer size
  auto sendBuffer = v.find("send_buffer");
  if (sendBuffer) {
    p->setSendBuffer(sendBuffer->as<int>());
  }

  return p;
}

std::unique_ptr<PacketPublisher> createPacketPublisher(const toml::Value& v) {
  auto bind = v.find("bind");
  if (!bind) {
    throw std::invalid_argument("Expected publisher section to have \"bind\" key");
  }

  // Only need bind value to create publisher
  auto p = PacketPublisher::create(bind->as<std::string>());

  // Optional send buffer size
  auto sendBuffer = v.find("send_buffer");
  if (sendBuffer) {
    p->setSendBuffer(sendBuffer->as<int>());
  }

  return p;
}

std::unique_ptr<StatsPublisher> createStatsPublisher(const toml::Value& v) {
  auto bind = v.find("bind");
  if (!bind) {
    throw std::invalid_argument("Expected publisher section to have \"bind\" key");
  }

  // Only need bind value to create publisher
  auto p = StatsPublisher::create(bind->as<std::string>());

  // Optional send buffer size
  auto sendBuffer = v.find("send_buffer");
  if (sendBuffer) {
    p->setSendBuffer(sendBuffer->as<int>());
  }

  return p;
}

void loadDemodulator(Config::Demodulator& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "mode") {
      out.downlinkType = value.as<std::string>();
      continue;
    }

    if (key == "source") {
      out.source = value.as<std::string>();
      continue;
    }

    if (key == "stats_publisher") {
      out.statsPublisher = createStatsPublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

void loadNanomsgSource(Config::Nanomsg& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "connect") {
      out.connect = value.as<std::string>();
      continue;
    }

    if (key == "receive_buffer") {
      out.receiveBuffer = value.as<int>();
      continue;
    }

    if (key == "sample_rate") {
      out.sampleRate = value.as<int>();
      continue;
    }

    throwInvalidKey(key);
  }

  if (out.sampleRate == 0) {
    std::stringstream ss;
    ss << "Key not set: sample_rate";
    throw std::invalid_argument(ss.str());
  }
}

void loadSource(Config::Source& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "sample_publisher") {
      out.samplePublisher = createSamplePublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

void loadAGC(Config::AGC& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "sample_publisher") {
      out.samplePublisher = createSamplePublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

void loadCostas(Config::Costas& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "sample_publisher") {
      out.samplePublisher = createSamplePublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

void loadRRC(Config::RRC& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "sample_publisher") {
      out.samplePublisher = createSamplePublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

void loadClockRecovery(Config::ClockRecovery& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "sample_publisher") {
      out.samplePublisher = createSamplePublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

void loadQuantization(Config::Quantization& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "soft_bit_publisher") {
      out.softBitPublisher = createSoftBitPublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

void loadDecoder(Config::Decoder& out, const toml::Value& v) {
  const auto& table = v.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "packet_publisher") {
      out.packetPublisher = createPacketPublisher(value);
      continue;
    }

    if (key == "stats_publisher") {
      out.statsPublisher = createStatsPublisher(value);
      continue;
    }

    throwInvalidKey(key);
  }
}

} // namespace

Config Config::load(const std::string& file) {
  Config out;

  auto pr = toml::parseFile(file);
  if (!pr.valid()) {
    throw std::invalid_argument(pr.errorReason);
  }

  const auto& table = pr.value.as<toml::Table>();
  for (const auto& it : table) {
    const auto& key = it.first;
    const auto& value = it.second;

    if (key == "demodulator") {
      loadDemodulator(out.demodulator, value);
      continue;
    }

    if (key == "nanomsg") {
      loadNanomsgSource(out.nanomsg, value);
      continue;
    }

    if (key == "source") {
      loadSource(out.source, value);
      continue;
    }

    if (key == "agc") {
      loadAGC(out.agc, value);
      continue;
    }

    if (key == "costas") {
      loadCostas(out.costas, value);
      continue;
    }

    if (key == "rrc") {
      loadRRC(out.rrc, value);
      continue;
    }

    if (key == "clock_recovery") {
      loadClockRecovery(out.clockRecovery, value);
      continue;
    }

    if (key == "quantization") {
      loadQuantization(out.quantization, value);
      continue;
    }

    if (key == "decoder") {
      loadDecoder(out.decoder, value);
      continue;
    }

    throwInvalidKey(key);
  }

  return out;
}
