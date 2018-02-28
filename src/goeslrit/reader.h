#pragma once

#include <array>

// Reader is an abstract base class for things that read packets.
class Reader {
public:
  Reader();
  virtual ~Reader();

  virtual bool nextPacket(std::array<uint8_t, 892>& out) = 0;
};
