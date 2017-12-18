#include <assert.h>
#include <string.h>

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "file_handler.h"
#include "vcdu.h"
#include "virtual_channel.h"

int main(int argc, char** argv) {
  // Turn argv into list of files
  std::vector<std::string> files;
  for (int i = 1; i < argc; i++) {
    files.push_back(argv[i]);
  }

  std::ifstream ifs;
  std::array<uint8_t, 892> buf;
  std::map<int, VirtualChannel> vcs;
  FileHandler handler("./out");
  for (;;) {
    // Make sure the ifstream is OK
    if (!ifs.good() || ifs.eof()) {
      if (files.size() == 0) {
        break;
      }
      // Open next file
      std::cerr << "Reading from: " << files.front() << std::endl;
      ifs.close();
      ifs.open(files.front());
      assert(ifs.good());
      files.erase(files.begin());
    }

    ifs.read((char*) buf.data(), buf.size());
    if (ifs.eof()) {
      continue;
    }

    // Got packet!
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
