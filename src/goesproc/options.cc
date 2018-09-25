#include "options.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <iostream>

#include "lib/dir.h"
#include "lib/version.h"

namespace {

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] [path...]\n", argv[0]);
  fprintf(stderr, "Process stream of packets (VCDUs) or list of LRIT files.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -c, --config PATH          Path to configuration file\n");
  fprintf(stderr, "  -m, --mode [packet|lrit]   Process stream of VCDU packets\n");
  fprintf(stderr, "                             or pre-assembled LRIT files\n");
  fprintf(stderr, "      --subscribe ADDR       Address of nanomsg publisher\n");
  fprintf(stderr, "                             (implies --mode packet)\n");
  fprintf(stderr, "  -f  --force                Overwrite existing output files\n");
  fprintf(stderr, "      --out DIR              Output directory\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Other:\n");
  fprintf(stderr, "      --help     Display this help and exit\n");
  fprintf(stderr, "      --version  Print version information and exit\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If mode is set to packet, goesproc reads VCDU packets from the\n");
  fprintf(stderr, "specified path(s). To process real time data you can either setup a pipe\n");
  fprintf(stderr, "from the decoder into goesproc (e.g. use /dev/stdin as path argument),\n");
  fprintf(stderr, "or use --subscribe to consume packets directly from goesrecv.\n");
  fprintf(stderr, "To process recorded data you can specify a list of files that contain\n");
  fprintf(stderr, "VCDU packets in chronological order.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If mode is set to lrit, goesproc finds all LRIT files in the specified\n");
  fprintf(stderr, "paths and processes them sequentially. You can specify a mix of files\n");
  fprintf(stderr, "and directories. Directory arguments expand into the files they\n");
  fprintf(stderr, "contain that match the glob '*.lrit*'. The complete list of LRIT files\n");
  fprintf(stderr, "is sorted according to their time stamp header prior to processing it.\n");
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
      {"config",    required_argument, nullptr, 'c'},
      {"mode",      required_argument, nullptr, 'm'},
      {"subscribe", required_argument, nullptr, 0x1001},
      {"force",     no_argument,       nullptr, 'f'},
      {"out",       required_argument, nullptr, 0x1003},
      {"help",      no_argument,       nullptr, 0x1337},
      {"version",   no_argument,       nullptr, 0x1338},
      {nullptr,     0,                 nullptr, 0},
    };

    auto c = getopt_long(argc, argv, "c:m:f", longOpts, nullptr);
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
        } else if (tmp == "lrit") {
          opts.mode = ProcessMode::LRIT;
        } else {
          fprintf(stderr, "%s: invalid argument '%s' for '--mode'\n", argv[0], tmp.c_str());
          fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
          exit(1);
        }
      }
      break;
    case 0x1001: // --subscribe
      opts.subscribe = std::string(optarg);
      // Specifying subscription address implies packet processing mode
      if (opts.mode == ProcessMode::UNDEFINED) {
        opts.mode = ProcessMode::PACKET;
      }
      break;
    case 'f':
      opts.force = true;
      break;
    case 0x1003: // --out
      opts.out = optarg;
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

  // Require process mode to be specified
  if (opts.mode == ProcessMode::UNDEFINED) {
    fprintf(stderr, "%s: no mode specified\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(1);
  }

  // If subscription address is specified we must be in packet processing mode
  if (!opts.subscribe.empty() && opts.mode != ProcessMode::PACKET) {
    fprintf(stderr, "%s: use of '--subscribe' implies '--mode packet'\n", argv[0]);
    fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
    exit(1);
  }

  // Collect paths
  for (int i = optind; i < argc; i++) {
    opts.paths.push_back(argv[i]);
  }

  // Sane default in packet mode
  if (opts.mode == ProcessMode::PACKET) {
    // Read from stdin if no paths were specified
    if (opts.paths.empty()) {
      // This only works on Linux...
      opts.paths.push_back("/proc/self/fd/0");
    } else {
      // If we're running in packet mode and encounter a directory
      // argument, it will be expanded into the list of *.raw files
      // present inside that directory.
      std::vector<std::string> files;
      for (const auto& path : opts.paths) {
        struct stat st;
        auto rv = stat(path.c_str(), &st);
        if (rv < 0) {
          perror("stat");
          exit(1);
        }
        if (S_ISDIR(st.st_mode)) {
          Dir dir(path);
          auto result = dir.matchFiles("*.raw");
          std::sort(result.begin(), result.end());
          files.insert(files.end(), result.begin(), result.end());
        } else {
          files.push_back(path);
        }
      }
      opts.paths = std::move(files);
    }
  }

  argc -= optind;
  argv = &argv[optind];
  return opts;
}
