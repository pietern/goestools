#include <assert.h>
#include <getopt.h>
#include <string.h>

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "file_handler.h"
#if HAVE_OPENSSL
#include "sha_handler.h"
#endif
#include "vcdu.h"
#include "virtual_channel.h"

struct Options {
  bool sha1 = false;

  static Options parse(int& argc, char**& argv) {
    Options opts;

    static struct option longOpts[] = {
#if HAVE_OPENSSL
      {"sha1", no_argument, 0, 'h'},
#endif
      {},
    };

    while (1) {
      int c = getopt_long(argc, argv, "h", longOpts, nullptr);
      if (c == -1) {
        break;
      }

      switch (c) {
      case 0:
        break;
#if HAVE_OPENSSL
      case 'h':
        opts.sha1 = true;
        break;
#endif
      default:
        std::cerr << "Invalid option" << std::endl;
        exit(1);
      }
    }

    argc -= optind;
    argv = &argv[optind];
    return opts;
  }
};

int main(int argc, char** argv) {
  auto opts = Options::parse(argc, argv);

  // Turn argv into list of files
  std::vector<std::string> files;
  for (int i = 0; i < argc; i++) {
    files.push_back(argv[i]);
  }

  std::ifstream ifs;
  std::array<uint8_t, 892> buf;
  std::map<int, VirtualChannel> vcs;
  FileHandler handler("./out");
#if HAVE_OPENSSL
  SHAHandler shaHandler;
#endif

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
#if HAVE_OPENSSL
      if (opts.sha1) {
        shaHandler.handle(std::move(spdu));
        continue;
      }
#endif

      handler.handle(std::move(spdu));
    }
  }
}
