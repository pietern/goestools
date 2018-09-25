#include "options.h"

#include <getopt.h>
#include <stdlib.h>

#include <iostream>

#include "lib/version.h"

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] [FILE...]\n", argv[0]);
  fprintf(stderr, "Assemble LRIT files from packet stream.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "      --subscribe ADDR  Address of nanomsg publisher\n");
  fprintf(stderr, "  -n, --dry-run         Don't write files\n");
  fprintf(stderr, "      --out DIR         Output directory\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Filtering:\n");
  fprintf(stderr, "      --all             Include everything\n");
  fprintf(stderr, "      --images          Include image files\n");
  fprintf(stderr, "      --messages        Include message files\n");
  fprintf(stderr, "      --text            Include text files\n");
  fprintf(stderr, "      --dcs             Include DCS files\n");
  fprintf(stderr, "      --emwin           Include EMWIN files\n");
  fprintf(stderr, "      --vcid VCID       Process only specified VCIDs\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Other:\n");
  fprintf(stderr, "      --help     Display this help and exit\n");
  fprintf(stderr, "      --version  Print version information and exit\n");
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
      {"out",       required_argument, nullptr, 0x1003},
      {"all",       no_argument,       nullptr, 0x1100},
      {"images",    no_argument,       nullptr, 0x1101},
      {"messages",  no_argument,       nullptr, 0x1102},
      {"text",      no_argument,       nullptr, 0x1103},
      {"dcs",       no_argument,       nullptr, 0x1104},
      {"emwin",     no_argument,       nullptr, 0x1105},
      {"vcid",      required_argument, nullptr, 0x1106},
      {"help",      no_argument,       nullptr, 0x1337},
      {"version",   no_argument,       nullptr, 0x1338},
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
    case 0x1003:
      opts.out = optarg;
      break;
    case 0x1100:
      opts.images = true;
      opts.messages = true;
      opts.text = true;
      opts.dcs = true;
      opts.emwin = true;
      break;
    case 0x1101:
      opts.images = true;
      break;
    case 0x1102:
      opts.messages = true;
      break;
    case 0x1103:
      opts.text = true;
      break;
    case 0x1104:
      opts.dcs = true;
      break;
    case 0x1105:
      opts.emwin = true;
      break;
    case 0x1106:
      opts.vcids.insert(std::stoi(optarg));
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
