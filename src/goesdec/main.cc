#include <assert.h>
#include <getopt.h>
#include <string.h>

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "file_handler.h"
#include "assembler/assembler.h"

int main(int argc, char** argv) {
  bool images = false;
  bool messages = false;
  bool text = false;
  bool dcs = false;

  for (;;) {
    static struct option longOpts[] = {
      {"images", no_argument, 0, 0x1001},
      {"messages", no_argument, 0, 0x1002},
      {"text", no_argument, 0, 0x1003},
      {"dcs", no_argument, 0, 0x1004},
    };

    int index;
    auto c = getopt_long(argc, argv, "", longOpts, &index);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 0x1001:
      images = true;
      break;
    case 0x1002:
      messages = true;
      break;
    case 0x1003:
      text = true;
      break;
    case 0x1004:
      dcs = true;
      break;
    default:
      std::cerr << "Invalid option" << std::endl;
      exit(1);
    }
  }

  if (!images && !messages && !text && !dcs) {
    std::cerr << "Specify one of --images, --messages, --text, --dcs" << std::endl;
    exit(1);
  }

  // Turn argv into list of files
  std::vector<std::string> files;
  for (int i = optind; i < argc; i++) {
    files.push_back(argv[i]);
  }

  std::ifstream ifs;
  std::array<uint8_t, 892> buf;
  assembler::Assembler assembler;
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

    // Got packet! Let packet assembler process it.
    auto spdus = assembler.process(buf);
    for (auto& spdu : spdus) {
      auto type = spdu->getHeader<lrit::PrimaryHeader>().fileType;
      if (type == 0 && !images) {
        continue;
      }
      if (type == 1 && !messages) {
        continue;
      }
      if (type == 2 && !text) {
        continue;
      }
      if (type == 130 && !dcs) {
        continue;
      }

      handler.handle(std::move(spdu));
    }
  }
}
