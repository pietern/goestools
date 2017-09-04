#include "options.h"

#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include "lib/lrit.h"
#include "dir.h"

Options parseOptions(int argc, char** argv) {
  Options opts;

  while (1) {
    static struct option longOpts[] = {
      {"channel", required_argument, 0, 'c'},
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
      opts.channel = optarg;
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
      auto result = dir.matchFiles("*.lrit");
      paths.insert(paths.end(), result.begin(), result.end());
    } else {
      paths.push_back(argv[i]);
    }
  }

  if (paths.empty()) {
    std::cerr << "No files to process..." << std::endl;
    exit(0);
  }

  // Load header of all specified files
  for (const auto& p : paths) {
    opts.files.push_back(File(p));
  }

  auto firstHeader = opts.files.front().getHeader<LRIT::PrimaryHeader>();
  opts.fileType = firstHeader.fileType;

  // Verify all files have the same fundamental type
  for (const auto& f : opts.files) {
    auto ph = f.getHeader<LRIT::PrimaryHeader>();
    if (ph.fileType != opts.fileType) {
      std::cerr << "Cannot handle mixed LRIT file types..." << std::endl;
      exit(1);
    }
  }

  return opts;
}
