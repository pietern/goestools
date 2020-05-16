#pragma once

#include <array>
#include <cstdint>

// PacketReader is an abstract base class for things that read packets.
class PacketReader {
public:
  PacketReader();
  virtual ~PacketReader();

  virtual bool nextPacket(std::array<uint8_t, 892>& out) = 0;
};
