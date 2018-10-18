#pragma once

#include <array>

// VCDU: Virtual Channel Data Unit.
// This is the data packet AFTER applying Reed Solomon decode.
class VCDU {
  using raw = std::array<uint8_t, 892>;

public:
  // Allow implicit construction
  VCDU(const raw& data)
    : data_(data) {
  };

  int getVersion() const {
    return (data_[0] & 0xc0) >> 6;
  }

  int getSCID() const {
    return (data_[0] & 0x3f) << 2 | (data_[1] & 0xc0) >> 6;
  }

  int getVCID() const {
    return (data_[1] & 0x3f);
  }

  int getCounter() const {
    return (data_[2] << 16) | (data_[3] << 8) | data_[4];
  }

  const uint8_t* data() const {
    return &data_[6];
  }

  size_t len() const {
    return data_.size() - 6;
  }

protected:
  const raw& data_;
};
