#include "qbt.h"

#include <util/error.h>

namespace qbt {

namespace {

#define CHAR_EQUAL(it, end, value) do {         \
    if ((it) != (end) && *(it) != (value)) {    \
      return false;                             \
    }                                           \
  } while (0)

bool isPacketPrefix(
  std::vector<uint8_t>::iterator it,
  std::vector<uint8_t>::iterator end) {
  // 6 byte prefix equal to 0x00
  CHAR_EQUAL(it + 0, end, 0x00);
  CHAR_EQUAL(it + 1, end, 0x00);
  CHAR_EQUAL(it + 2, end, 0x00);
  CHAR_EQUAL(it + 3, end, 0x00);
  CHAR_EQUAL(it + 4, end, 0x00);
  CHAR_EQUAL(it + 5, end, 0x00);

  // Product Filename
  CHAR_EQUAL(it + 6, end, '/');
  CHAR_EQUAL(it + 7, end, 'P');
  CHAR_EQUAL(it + 8, end, 'F');

  // Packet Number
  CHAR_EQUAL(it + 21, end, '/');
  CHAR_EQUAL(it + 22, end, 'P');
  CHAR_EQUAL(it + 23, end, 'N');

  // Packets Total
  CHAR_EQUAL(it + 30, end, '/');
  CHAR_EQUAL(it + 31, end, 'P');
  CHAR_EQUAL(it + 32, end, 'T');

  // Computed Sum
  CHAR_EQUAL(it + 39, end, '/');
  CHAR_EQUAL(it + 40, end, 'C');
  CHAR_EQUAL(it + 41, end, 'S');

  // If none of these failed it must be a valid packet prefix
  return true;
}

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

} // namespace

std::unique_ptr<Packet> Assembler::process(qbt::Fragment f) {
  std::unique_ptr<Packet> out;
  auto counter = f.counter();
  auto skip = diffWithWrap<(1 << 16)>(counter_, counter);
  counter_ = counter;

  // Abort processing of pending packet if we skipped a packet
  if (skip > 1) {
    tmp_.clear();
  }

  // Append contents of S_PDU to temporary buffer
  const auto& buf = f.data();
  tmp_.insert(tmp_.end(), buf.begin(), buf.end());

  // Find prefix of QBT packet
  auto it = tmp_.begin();
  while (it != tmp_.end()) {
    if (isPacketPrefix(it, tmp_.end())) {
      break;
    }
    it++;
  }

  // Remove everything up to the packet prefix
  if (it != tmp_.begin()) {
    tmp_.erase(tmp_.begin(), it);
  }

  // There is a valid packet prefix and a temporary buffer of at
  // least the size of a single QBT packet, so we can return it.
  if (tmp_.size() >= 1116) {
    std::array<uint8_t, 1116> buf;
    std::copy(tmp_.begin(), tmp_.begin() + 1116, buf.begin());
    tmp_.erase(tmp_.begin(), tmp_.begin() + 1116);
    return std::unique_ptr<Packet>(new Packet(std::move(buf)));
  }

  return std::unique_ptr<Packet>();
}

} // namespace qbt
