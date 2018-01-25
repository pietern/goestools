#pragma once

#include <stdint.h>
#include <vector>

#include <unistd.h>

namespace assembler {

// Transport Protocol Data Unit
// See: http://www.noaasis.noaa.gov/LRIT/pdf-files/3_LRIT_Receiver-specs.pdf
class TransportPDU {
public:
  static constexpr auto headerBytes = 6;
  static constexpr auto dataBytes = 8192;

  TransportPDU() {
    header.reserve(headerBytes);
    data.reserve(dataBytes);
  }

  size_t read(const uint8_t* buf, size_t len);

  bool headerComplete() {
    return (header.size() == headerBytes);
  }

  bool dataComplete() {
    return headerComplete() && (data.size() == length());
  }

  // Packet Identification
  unsigned version() const {
    return (header[0] >> 5) & 0x7;
  }

  unsigned type() const {
    return (header[0] >> 4) & 0x1;
  }

  unsigned secondaryHeaderFlag() const {
    return (header[0] >> 3) & 0x1;
  }

  unsigned apid() const {
    return ((header[0] & 0x7) << 8) | header[1];
  }

  // Packet Sequence Control
  unsigned sequenceFlag() const {
    return (header[2] >> 6) & 0x3;
  }

  unsigned sequenceCount() const {
    return (header[2] & 0x3f) << 8 | header[3];
  }

  // Packet Length
  uint16_t length() const {
    // This field is specified as (see 6.2.1 of LRIT transmitter spec):
    //
    // > 16-bit binary count that expresses the length of the
    // > remainder of the source packet following this field minus 1.
    //
    // To compensate, we add 1 to get back the length in bytes.
    //
    return ((header[4] << 8) | header[5]) + 1;
  }

  uint16_t crc() const {
    const uint8_t* b = &data[length()-2];
    return (b[0] << 8) | b[1];
  }

  bool verifyCRC();

  std::vector<uint8_t> header;
  std::vector<uint8_t> data;
};

} // namespace assembler
