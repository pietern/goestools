#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace qbt {

class Fragment {
  using raw = std::array<uint8_t, 836>;

public:
  Fragment(
      uint16_t counter,
      const std::vector<uint8_t>::const_iterator& begin,
      const std::vector<uint8_t>::const_iterator& end)
      : counter_(counter) {
    if ((size_t)(end - begin) > data_.size()) {
      throw std::runtime_error("range too large");
    }
    std::copy(begin, end, data_.begin());
  }

  Fragment(Fragment&& other) = default;

  uint16_t counter() const {
    return counter_;
  }

  const raw& data() const {
    return data_;
  }

protected:
  uint16_t counter_;
  raw data_;
};

// For overview of QBT structure, see http://www.nws.noaa.gov/emwin/EMWIN%20QBT%20Satellite%20Broadcast%20Protocol%20draft%20v1.0.3.pdf
class Packet {
  using raw = std::array<uint8_t, 1116>;

public:
  Packet(raw data) : data_(data) {
  }

  std::string filename() const {
    auto str = std::string(data_.begin() + 9, data_.begin() + 21);
    auto pos = str.find('.');
    return str.substr(0, pos + 4);
  }

  unsigned long packetNumber() const {
    auto str = std::string(data_.begin() + 24, data_.begin() + 30);
    return std::stoul(str);
  }

  unsigned long packetTotal() const {
    auto str = std::string(data_.begin() + 33, data_.begin() + 39);
    return std::stoul(str);
  }

  const raw& data() const {
    return data_;
  }

  std::vector<uint8_t> payload() const {
    std::vector<uint8_t> out;
    out.insert(out.end(), data_.begin() + 86, data_.begin() + 1110);
    return out;
  }

protected:
  raw data_;
};

class Assembler {
public:
  Assembler() : counter_(0) {
  }

  // QBT packets are 1116 bytes.
  // The data in an S_PDU on the LRIT stream is 836 bytes.
  // Therefore, an S_PDU contains part of 1 or 2 QBT packets.
  std::unique_ptr<Packet> process(qbt::Fragment f);

protected:
  uint16_t counter_;
  std::vector<uint8_t> tmp_;
};

} // namespace qbt
