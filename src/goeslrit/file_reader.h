#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "reader.h"

class FileReader : public Reader {
public:
  FileReader(const std::vector<std::string>& files);
  virtual ~FileReader();

  virtual bool nextPacket(std::array<uint8_t, 892>& out);

protected:
  std::vector<std::string> files_;
  std::ifstream ifs_;
};
