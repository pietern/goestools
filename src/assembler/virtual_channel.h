#pragma once

#include <map>
#include <memory>
#include <vector>

#include "session_pdu.h"
#include "transport_pdu.h"
#include "vcdu.h"

namespace assembler {

class VirtualChannel {
public:
  explicit VirtualChannel(int id);

  // For every packet processed, we may get back multiple completed
  // Session PDUs for further processing.
  std::vector<std::unique_ptr<SessionPDU>> process(const VCDU& p);

protected:
  void process(
    std::unique_ptr<TransportPDU> tpdu,
    std::vector<std::unique_ptr<SessionPDU>>& out);

  void finish(
    std::unique_ptr<SessionPDU> spdu,
    std::vector<std::unique_ptr<SessionPDU>>& out);

  int id_;
  int n_;

  // Incomplete Transport Protocol Data Unit.
  std::unique_ptr<TransportPDU> tpdu_;

  // Sequence number by APID. Used to detect drops.
  std::map<int, int> apidSeq_;

  // Incomplete Session Protocol Data Unit per APID.
  std::map<int, std::unique_ptr<SessionPDU>> apidSessionPDU_;
};

} // namespace assembler
