#pragma once

#include <map>

#include "assembler/virtual_channel.h"

namespace assembler {

// Assembler takes in a stream of VCDUs and dispatches
// them to the appropriate virtual channels.
class Assembler {
public:
  explicit Assembler();

  // For every packet processed, we may get back multiple completed
  // Session PDUs for further processing.
  std::vector<std::unique_ptr<SessionPDU>> process(const VCDU& p);

protected:
  std::map<int, VirtualChannel> vcs_;
};

} // namespace assembler
