#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

extern "C" {
#include <szlib.h>
}

#include "lrit/lrit.h"

#include "transport_pdu.h"

// Compute difference between two integers taking into account wrapping.
template <unsigned int N>
int diffWithWrap(unsigned int a, unsigned int b) {
  int skip;

  assert(a < N);
  assert(b < N);

  if (a <= b) {
    skip = b - a;
  } else {
    skip = N - a + b;
  }

  return skip;
}

class SessionPDU {
public:
  explicit SessionPDU(const TransportPDU& tpdu);

  bool append(const TransportPDU& tpdu);

  // Whether or not we can skip a couple of packets for this session PDU.
  // This is only the case if the LRIT header was already received
  // and we're dealing with a line-by-line encoded image.
  bool canResumeFrom(const TransportPDU& tpdu) const;

  std::string getName() const;

  bool hasCompleteHeader() const {
    return !m_.empty();
  }

  template <typename H>
  bool hasHeader() const {
    return LRIT::hasHeader<H>(m_);
  }

  template <typename H>
  H getHeader() const {
    return LRIT::getHeader<H>(buf_, m_);
  }

  const std::vector<uint8_t>& get() const {
    return buf_;
  }

  const size_t size() const {
    return buf_.size();
  }

  const LRIT::HeaderMap& getHeaderMap() const {
    return m_;
  }

protected:
  void completeHeader();

  bool append(
    std::vector<uint8_t>::const_iterator begin,
    std::vector<uint8_t>::const_iterator end);

  std::vector<uint8_t> buf_;
  uint64_t remainingHeaderBytes_;
  uint32_t lastSequenceCount_;

  LRIT::HeaderMap m_;
  LRIT::PrimaryHeader ph_;
  LRIT::ImageStructureHeader ish_;

  std::unique_ptr<SZ_com_t> szParam_;
  std::vector<uint8_t> szTmp_;
};
