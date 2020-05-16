#pragma once

#include <array>
#include <cstdint>
#include <ctime>

// PacketWriter is an abstract base class for things that write packets.
class PacketWriter {
public:
  PacketWriter();
  virtual ~PacketWriter();

  virtual void write(const std::array<uint8_t, 892>& in, time_t t) = 0;
};
