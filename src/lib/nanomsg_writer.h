#pragma once

#include <string>
#include <vector>

#include "packet_writer.h"

class NanomsgWriter : public PacketWriter {
public:
  NanomsgWriter(const std::vector<std::string>& endpoints);
  virtual ~NanomsgWriter();

  virtual void write(const std::array<uint8_t, 892>& in, time_t t);

protected:
  int fd_;
};
