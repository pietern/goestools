#include "assembler.h"

namespace assembler {

Assembler::Assembler() {
}

std::vector<std::unique_ptr<SessionPDU>> Assembler::process(const VCDU& vcdu) {
  std::vector<std::unique_ptr<SessionPDU>> out;

  // Ignore fill packets
  auto vcid = vcdu.getVCID();
  if (vcid == 63) {
    return out;
  }

  // Create virtual channel instance if it does not yet exist
  if (vcs_.find(vcid) == vcs_.end()) {
    vcs_.insert(std::make_pair(vcid, VirtualChannel(vcid)));
  }

  // Let virtual channel process VCDU
  auto it = vcs_.find(vcid);
  return it->second.process(vcdu);
}

} // namespace assembler
