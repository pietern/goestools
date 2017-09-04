#pragma once

#include <array>
#include <stdint.h>

class packet {
  // Make this class quack like a std::array
  using value_type = uint8_t;
  using size_type = uint32_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;

  // Actual contents
  using raw_type = std::array<uint8_t, 892>;

public:
  packet() {
  }

  packet(raw_type& p) : p_(p) {
  }

  reference operator[](size_type pos) {
    return p_[pos];
  }

  const_reference operator[](size_type pos) const {
    return p_[pos];
  }

  size_type size() const noexcept {
    return p_.size();
  }

  int version() const {
    return (p_[0] & 0xc0) >> 6;
  }

  int sc_id() const {
    return (p_[0] & 0x3f) << 2 | (p_[1] & 0xc0) >> 6;
  }

  int vc_id() const {
    return (p_[1] & 0x3f);
  }

  int counter() const {
    return (p_[2] << 16) | (p_[3] << 8) | p_[4];
  }

protected:
  raw_type p_;
};
