#include "options.h"

#include <getopt.h>
#include <stdlib.h>

#include <iostream>

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", argv[0]);
  fprintf(stderr, "Extract EMWIN data from packet stream.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "      --subscribe ADDR  Address of nanomsg publisher\n");
  fprintf(stderr, "      --mode MODE       One of raw, qbt, or emwin (default: raw)\n");
  fprintf(stderr, "      --out DIR         Output directory\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If a nanomsg address to subscribe to is specified,\n");
  fprintf(stderr, "FILE arguments are not used.\n");
  fprintf(stderr, "\n");
  exit(0);
}

Options parseOptions(int argc, char** argv) {
  Options opts;

  while (1) {
    static struct option longOpts[] = {
      {"subscribe", required_argument, nullptr, 0x1001},
      {"mode",      required_argument, nullptr, 0x1002},
      {"out",       required_argument, nullptr, 0x1003},
      {"help",      no_argument,       nullptr, 0x1337},
      {nullptr,     0,                 nullptr, 0},
    };

    auto c = getopt_long(argc, argv, "n", longOpts, nullptr);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 0x1001:
      opts.nanomsg = optarg;
      break;
    case 0x1002:
      {
        auto tmp = std::string(optarg);
        if (tmp == "raw") {
          opts.mode = Mode::RAW;
        } else if (tmp == "qbt") {
          opts.mode = Mode::QBT;
        } else if (tmp == "emwin") {
          opts.mode = Mode::EMWIN;
        }
      }
      break;
    case 0x1003:
      opts.out = optarg;
      break;
    case 0x1337:
      usage(argc, argv);
      break;
    default:
      std::cerr << "Invalid option" << std::endl;
      exit(1);
    }
  }

  for (int i = optind; i < argc; i++) {
    opts.files.push_back(argv[i]);
  }

  return opts;
}
