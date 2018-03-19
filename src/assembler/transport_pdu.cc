#include "transport_pdu.h"

#include <cstring>

#include "crc.h"

namespace assembler {

size_t TransportPDU::read(const uint8_t* buf, size_t len) {
  size_t nread = 0;

  // Read remaining header
  if (header.size() < headerBytes) {
    auto remaining = headerBytes - header.size();
    auto hpos = header.size();
    auto hread = std::min(remaining, (len-nread));
    header.resize(hpos + hread);
    memcpy(&header[hpos], &buf[nread], hread);
    nread += hread;
  }

  // Read remaining user data (if there is more data)
  if (nread < len) {
    if (data.size() < length()) {
      auto remaining = length() - data.size();
      auto dpos = data.size();
      auto dread = std::min(remaining, (len-nread));
      data.resize(dpos + dread);
      memcpy(&data[dpos], &buf[nread], dread);
      nread += dread;
    }
  }

  return nread;
}

bool TransportPDU::verifyCRC() {
  if (length() >= 2) {
    return (::assembler::crc(&data[0], length() - 2) == this->crc());
  } else {
    return false;
  }
}

} // namespace assembler
