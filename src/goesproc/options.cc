#include "options.h"

#include <stdlib.h>
#include <getopt.h>

#include <iostream>

namespace {

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] [path...]\n", argv[0]);
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c, --config PATH          Path to configuration file\n");
  fprintf(stderr, "  -m, --mode [packet|lrit]   Process stream of VCDU packets\n");
  fprintf(stderr, "                             or pre-assembled LRIT files\n");
  fprintf(stderr, "      --help                 Show this help\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If mode is set to packet, goesproc reads VCDU packets from the");
  fprintf(stderr, "specified path(s). To process real time data you can setup a pipe from");
  fprintf(stderr, "the decoder into goesproc (e.g. use /dev/stdin as path argument).");
  fprintf(stderr, "To process recorded data you can specify a list of files that contain");
  fprintf(stderr, "VCDU packets in chronological order.");
  fprintf(stderr, "\n");
  fprintf(stderr, "If mode is set to lrit, goesproc finds all LRIT files in the specified");
  fprintf(stderr, "paths and processes them sequentially. You can specify a mix of files");
  fprintf(stderr, "and directories. Directory arguments expand into the files they");
  fprintf(stderr, "contain that match the glob '*.lrit*'. The complete list of LRIT files");
  fprintf(stderr, "is sorted according to their time stamp header prior to processing it.");
  fprintf(stderr, "\n");
  exit(0);
}

} // namespace

Options parseOptions(int& argc, char**& argv) {
  Options opts;

  // Defaults
  opts.config = "";
  opts.mode = ProcessMode::UNDEFINED;

  while (1) {
    static struct option longOpts[] = {
      {"config", required_argument, 0, 'c'},
      {"mode", required_argument, 0, 'm'},
      {"help", required_argument, 0, 0x1337},
    };
    int i;
    int c = getopt_long(argc, argv, "c:m:", longOpts, &i);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 'c':
      opts.config = optarg;
      break;
    case 'm':
      {
        auto tmp = std::string(optarg);
        if (tmp == "packet") {
          opts.mode = ProcessMode::PACKET;
        }
        if (tmp == "lrit") {
          opts.mode = ProcessMode::LRIT;
        }
      }
      break;
    case 0x1337:
      usage(argc, argv);
      break;
    default:
      std::cerr << "Invalid option" << std::endl;
      exit(1);
    }
  }

  // Require configuration to be specified
  if (opts.config.empty()) {
    fprintf(stderr, "%s: no configuration file specified\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(1);
  }

  // Require process mode to be specified
  if (opts.mode == ProcessMode::UNDEFINED) {
    fprintf(stderr, "%s: no mode specified\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(1);
  }

  argc -= optind;
  argv = &argv[optind];
  return opts;
}
