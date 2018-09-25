#include "options.h"

#include <getopt.h>
#include <stdlib.h>

#include <iostream>

#include "lib/version.h"

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", argv[0]);
  fprintf(stderr, "Relay and/or record packet stream.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "      --subscribe ADDR    Address to subscribe to\n");
  fprintf(stderr, "      --publish ADDR      Address to re-publish packets to\n");
  fprintf(stderr, "      --vcid VCID         Virtual Channel ID to filter\n");
  fprintf(stderr, "                          (can be specified multiple times)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Record packet stream:\n");
  fprintf(stderr, "      --record            Enable recording of packet stream to disk\n");
  fprintf(stderr, "      --filename PATTERN  Filename pattern for packet files (see strftime(3))\n");
  fprintf(stderr, "                          (default: ./packets-%%FT%%H:%%M:00.raw)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Other:\n");
  fprintf(stderr, "      --help     Display this help and exit\n");
  fprintf(stderr, "      --version  Print version information and exit\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If an address to subscribe to is specified,\n");
  fprintf(stderr, "FILE arguments are ignored.\n");
  fprintf(stderr, "\n");
  exit(0);
}

Options parseOptions(int argc, char** argv) {
  Options opts;

  while (1) {
    static struct option longOpts[] = {
      {"subscribe", required_argument, 0,       0x1001},
      {"vcid",      required_argument, 0,       0x1002},
      {"publish",   required_argument, 0,       0x1003},
      {"record" ,   no_argument,       0,       0x1004},
      {"filename" , required_argument, 0,       0x1005},
      {"help",      no_argument,       nullptr, 0x1337},
      {"version",   no_argument,       nullptr, 0x1338},
      {nullptr,     0,                 nullptr, 0},
    };

    auto c = getopt_long(argc, argv, "", longOpts, nullptr);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 0x1001:
      opts.subscribe = optarg;
      break;
    case 0x1002:
      opts.vcids.insert(atoi(optarg));
      break;
    case 0x1003:
      opts.publish.push_back(optarg);
      break;
    case 0x1004:
      opts.record = true;
      break;
    case 0x1005:
      opts.filename = optarg;
      break;
    case 0x1337:
      usage(argc, argv);
      break;
    case 0x1338:
      version(argc, argv);
      exit(0);
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
