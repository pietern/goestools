#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

extern "C" {
#include <szlib.h>
}

#include <util/error.h>

#include "lrit/lrit.h"

#include "transport_pdu.h"

namespace assembler {

// Compute difference between two integers taking into account wrapping.
template <unsigned int N>
int diffWithWrap(unsigned int a, unsigned int b) {
  int skip;

  ASSERT(a < N);
  ASSERT(b < N);

  if (a <= b) {
    skip = b - a;
  } else {
    skip = N - a + b;
  }

  return skip;
}

class SessionPDU {
public:
  explicit SessionPDU(int vcid, int apid);

  // Returns false if this T_PDU could not be added.
  // This is the case if -- for example -- it contains a
  // malformed header, or cannot be decompressed.
  bool append(const TransportPDU& tpdu);

  // Returns true if this session PDU could be finished
  // without extra T_PDUs. This is only the case if it
  // contains a line-by-line encoded image.
  bool finish();

  std::string getName() const;

  bool hasCompleteHeader() const {
    return !m_.empty();
  }

  template <typename H>
  bool hasHeader() const {
    return lrit::hasHeader<H>(m_);
  }

  template <typename H>
  H getHeader() const {
    return lrit::getHeader<H>(buf_, m_);
  }

  const std::vector<uint8_t>& get() const {
    return buf_;
  }

  const size_t size() const {
    return buf_.size();
  }

  const lrit::HeaderMap& getHeaderMap() const {
    return m_;
  }

  const lrit::PrimaryHeader& getPrimaryHeader() const {
    return ph_;
  }

  const int vcid;
  const int apid;

protected:
  bool completeHeader();

  bool append(
    std::vector<uint8_t>::const_iterator begin,
    std::vector<uint8_t>::const_iterator end);

  std::vector<uint8_t> buf_;
  uint64_t remainingHeaderBytes_;
  uint32_t lastSequenceCount_;

  lrit::HeaderMap m_;
  lrit::PrimaryHeader ph_;
  lrit::ImageStructureHeader ish_;

  std::unique_ptr<SZ_com_t> szParam_;
  std::vector<uint8_t> szTmp_;

private:
  void skipLines(int skip);

  // Number of lines that are present in the buffer.
  // Only applicable for line-by-line encoded images.
  uint32_t linesDone_;
};

} // namespace assembler
