#include "options.h"

#include <getopt.h>

#include <iostream>

Options parseOptions(int argc, char** argv) {
  Options opts;

  while (1) {
    static struct option longOpts[] = {
      {"device", required_argument, 0, 'd'},
      {"lrit", no_argument, 0, 0x1001},
      {"hrit", no_argument, 0, 0x1002},
    };

    auto c = getopt_long(argc, argv, "d:", longOpts, nullptr);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 'd':
      opts.device = optarg;
      break;
    case 0x1001:
      opts.downlinkType = "lrit";
      break;
    case 0x1002:
      opts.downlinkType = "hrit";
      break;
    default:
      std::cerr << "Invalid option" << std::endl;
      exit(1);
    }
  }

  // Ensure downlink type is specified
  if (opts.downlinkType.empty()) {
    std::cerr << "Please specify either --lrit or --hrit" << std::endl;
    exit(1);
  }

  return opts;
}
