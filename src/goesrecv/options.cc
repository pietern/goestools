#include "options.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>

#include "lib/version.h"

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS]\n", argv[0]);
  fprintf(stderr, "Demodulate and decode signal into packet stream.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c, --config PATH          Path to configuration file\n");
  fprintf(stderr, "  -v, --verbose              Periodically show statistics\n");
  fprintf(stderr, "  -i, --interval SEC         Interval for --verbose\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Other:\n");
  fprintf(stderr, "      --help     Display this help and exit\n");
  fprintf(stderr, "      --version  Print version information and exit\n");
  fprintf(stderr, "\n");
  exit(0);
}

Options parseOptions(int argc, char** argv) {
  Options opts;

  while (1) {
    static struct option longOpts[] = {
      {"config",   required_argument, nullptr, 'c'},
      {"verbose",  no_argument,       nullptr, 'v'},
      {"interval", required_argument, nullptr, 'i'},
      {"help",     no_argument,       nullptr, 0x1337},
      {"version",  no_argument,       nullptr, 0x1338},
      {nullptr,    0,                 nullptr, 0},
    };

    auto c = getopt_long(argc, argv, "c:vi:", longOpts, nullptr);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 'c':
      opts.config = optarg;
      break;
    case 'v':
      opts.verbose = true;
      break;
    case 'i':
      opts.interval = std::chrono::milliseconds((int) (1000 * atof(optarg)));
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

  // Require configuration to be specified
  if (opts.config.empty()) {
    fprintf(stderr, "%s: no configuration file specified\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(1);
  }

  // Require configuration to be a regular file
  {
    struct stat st;
    const char* error = nullptr;
    auto rv = stat(opts.config.c_str(), &st);
    if (rv < 0) {
      error = strerror(errno);
    } else {
      if (!S_ISREG(st.st_mode)) {
        error = "Not a file";
      }
    }
    if (error != nullptr) {
      fprintf(stderr,
              "%s: invalid configuration file '%s': %s\n",
              argv[0],
              opts.config.c_str(),
              error);
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      exit(1);
    }
  }

  if (opts.verbose && opts.interval.count() == 0) {
    opts.interval = std::chrono::seconds(1);
  }

  return opts;
}
