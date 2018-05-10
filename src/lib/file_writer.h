#pragma once

#include <fstream>
#include <string>

#include "packet_writer.h"

class FileWriter : public PacketWriter {
public:
  FileWriter(const std::string& path);
  virtual ~FileWriter();

  virtual void write(const std::array<uint8_t, 892>& in, time_t t);

protected:
  std::string timeToFileName(time_t t);

  std::string path_;
  std::string fileName_;
  time_t fileTime_;
  std::ofstream of_;
};
