#pragma once

#include <string>

#include "packet_reader.h"

class NanomsgReader : public PacketReader {
public:
  NanomsgReader(const std::string& uri);
  virtual ~NanomsgReader();

  virtual bool nextPacket(std::array<uint8_t, 892>& out);

protected:
  int fd_;
};
