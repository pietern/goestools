#pragma once

#include <fstream>
#include <string>

#include "packet_writer.h"

class FileWriter : public PacketWriter {
public:
  FileWriter(const std::string& pattern);
  virtual ~FileWriter();

  virtual void write(const std::array<uint8_t, 892>& in, time_t t);

protected:
  const std::string pattern_;

  std::string buildFilename(time_t t);

  std::string currentFilename_;
  std::ofstream of_;
};
