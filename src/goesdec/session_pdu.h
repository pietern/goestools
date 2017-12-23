#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

extern "C" {
#include <szlib.h>
}

#include <lib/lrit.h>

#include "transport_pdu.h"

class SessionPDU {
public:
  explicit SessionPDU(const TransportPDU& tpdu);

  bool append(const TransportPDU& tpdu);

  template <typename H>
  bool hasHeader() {
    return LRIT::hasHeader<H>(m_);
  }

  template <typename H>
  H getHeader() {
    return LRIT::getHeader<H>(buf_, m_);
  }

  const std::vector<uint8_t>& get() const {
    return buf_;
  }

  const size_t size() const {
    return buf_.size();
  }

  const LRIT::HeaderMap& getHeaderMap() {
    return m_;
  }

protected:
  void completeHeader();

  bool append(
    std::vector<uint8_t>::const_iterator begin,
    std::vector<uint8_t>::const_iterator end);

  std::vector<uint8_t> buf_;
  uint64_t remainingHeaderBytes_;

  LRIT::HeaderMap m_;
  LRIT::PrimaryHeader ph_;
  LRIT::ImageStructureHeader ish_;

  std::unique_ptr<SZ_com_t> szParam_;
  std::vector<uint8_t> szTmp_;
};
