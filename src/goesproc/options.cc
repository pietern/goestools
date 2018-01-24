#include "options.h"

#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <limits>

#include "lib/lrit.h"
#include "lib/util.h"
#include "dir.h"

namespace {

bool parseTime(const std::string& str, time_t* out) {
  const char* buf = str.c_str();
  struct tm tm;
  char* pos = strptime(buf, "%Y-%m-%d %H:%M:%S", &tm);
  if (pos < (buf + str.size())) {
    return false;
  }
  *out = mktime(&tm);
  return true;
}

} // namespace

Options parseOptions(int argc, char** argv) {
  Options opts;

  // Defaults
  opts.shrink = false;
  opts.format = "pgm";
  opts.start = 0;
  opts.stop = std::numeric_limits<time_t>::max();
  opts.intervalSet = false;

  while (1) {
    static struct option longOpts[] = {
      {"channel", required_argument, 0, 'c'},
      {"shrink", no_argument, 0, 0x1001},
      {"scale", required_argument, 0, 0x1002},
      {"format", required_argument, 0, 0x1003},
      {"start", required_argument, 0, 0x1004},
      {"stop", required_argument, 0, 0x1005},
    };
    int i;
    int c = getopt_long(argc, argv, "c:", longOpts, &i);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 'c':
      opts.channels.push_back(optarg);
      break;
    case 0x1001: // --shrink
      opts.shrink = true;
      break;
    case 0x1002: // --scale
      {
        auto parts = split(optarg, 'x');
        if (parts.size() != 2) {
          std::cerr << "Invalid argument to --scale" << std::endl;
          exit(1);
        }
        opts.scale.width = std::stoi(parts[0]);
        opts.scale.height = std::stoi(parts[1]);
        opts.scale.cropWidth = CropWidth::CENTER;
        opts.scale.cropHeight = CropHeight::CENTER;
      }
      break;
    case 0x1003: // --format
      opts.format = optarg;
      break;
    case 0x1004: // --start
      if (!parseTime(optarg, &opts.start)) {
        std::cerr << "Invalid argument to --start" << std::endl;
        exit(1);
      }
      opts.intervalSet = true;
      break;
    case 0x1005: // --stop
      if (!parseTime(optarg, &opts.stop)) {
        std::cerr << "Invalid argument to --stop" << std::endl;
        exit(1);
      }
      opts.intervalSet = true;
      break;
    default:
      std::cerr << "Invalid option" << std::endl;
      exit(1);
    }
  }

  // Gather file names from arguments (globs *.lrit in directories).
  std::vector<std::string> paths;
  for (int i = optind; i < argc; i++) {
    struct stat st;
    auto rv = stat(argv[i], &st);
    if (rv < 0) {
      perror("stat");
      exit(1);
    }
    if (S_ISDIR(st.st_mode)) {
      Dir dir(argv[i]);
      auto result = dir.matchFiles("*.lrit*");
      paths.insert(paths.end(), result.begin(), result.end());
    } else {
      paths.push_back(argv[i]);
    }
  }

  if (paths.empty()) {
    std::cerr << "No files to process..." << std::endl;
    exit(0);
  }

  // Process and filter files
  for (unsigned i = 0; i < paths.size(); i++) {
    auto f = LRIT::File(paths[i]);
    auto ph = f.getHeader<LRIT::PrimaryHeader>();
    if (i == 0) {
      opts.fileType = ph.fileType;
    } else {
      // Verify all files have the same fundamental type
      if (ph.fileType != opts.fileType) {
        std::cerr << "Cannot handle mixed LRIT file types..." << std::endl;
        exit(1);
      }
    }

    if (f.hasHeader<LRIT::TimeStampHeader>()) {
      auto ts = f.getHeader<LRIT::TimeStampHeader>().getUnix();
      if (ts.tv_sec < opts.start || ts.tv_sec >= opts.stop) {
        continue;
      }
    } else {
      // No time stamp header but time restrictions configured
      if (opts.intervalSet) {
        continue;
      }
    }

    opts.files.push_back(std::move(f));
  }

  return opts;
}
