#include <assert.h>
#include <malloc.h>
#include <string.h>

#include <array>
#include <iostream>
#include <map>

#include "packetizer.h"
#include "file_handler.h"
#include "vcdu.h"
#include "virtual_channel.h"

int main(int argc, char** argv) {
  // Turn argv into list of files
  std::vector<std::string> files;
  for (int i = 1; i < argc; i++) {
    files.push_back(argv[i]);
  }

  FileHandler handler("./out");
  Packetizer p(std::make_unique<MultiFileReader>(files));
  std::array<uint8_t, 892> buf;

  std::map<int, VirtualChannel> vcs;
  for (auto j = 0; ; j++) {
    p.nextPacket(buf, nullptr);
    VCDU vcdu(buf);

    // Ignore fill packets
    auto vcid = vcdu.getVCID();
    if (vcid == 63) {
      continue;
    }

    // Create virtual channel instance if it does not yet exist
    if (vcs.find(vcid) == vcs.end()) {
      vcs.insert(std::make_pair(vcid, VirtualChannel(vcid)));
    }

    // Let virtual channel process VCDU
    auto it = vcs.find(vcid);
    auto spdus = it->second.process(vcdu);
    for (auto& spdu : spdus) {
      handler.handle(std::move(spdu));
    }
  }
}
