#include "options.h"

#include <getopt.h>
#include <stdlib.h>

#include <iostream>

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", argv[0]);
  fprintf(stderr, "Extract EMWIN packets from packet stream.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "      --subscribe ADDR       Address of nanomsg publisher\n");
  fprintf(stderr, "  -n, --dry-run              Don't write files\n");
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
      {"dry-run",   no_argument,       nullptr, 'n'},
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
    case 'n':
      opts.dryrun = true;
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
