#pragma once

#include <unistd.h>

#include <ctime>
#include <string>
#include <vector>

namespace dcs {

// Header at the beginning of an LRIT DCS file
struct FileHeader {
  std::string name;
  uint32_t length;

  // Don't know what these hold.
  // First one looks ASCII and second one looks binary.
  std::string misc1;
  std::vector<uint8_t> misc2;

  int readFrom(const char* buf, size_t len);
};

// Header of every single DCS payload
struct Header {
  uint64_t address;
  struct timespec time;
  char failure;
  int signalStrength;
  int frequencyOffset;
  char modulationIndex;
  char dataQuality;
  int receiveChannel;
  char spacecraft;
  char dataSourceCode[2];
  int dataLength;

  int readFrom(const char* buf, size_t len);
};

} // namespace dcs
