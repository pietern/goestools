#include "options.h"

#include <getopt.h>
#include <stdlib.h>

#include <iostream>

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", argv[0]);
  fprintf(stderr, "Relay and/or record packet stream.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "      --subscribe ADDR  Address to subscribe to\n");
  fprintf(stderr, "      --publish ADDR    Address to re-publish packets to\n");
  fprintf(stderr, "      --record          Write packet stream to files on disk\n");
  fprintf(stderr, "      --vcid VCID       Virtual Channel ID to filter\n");
  fprintf(stderr, "                        (can be specified multiple times)\n");
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
      {"help",      no_argument,       0,       0x1337},
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
