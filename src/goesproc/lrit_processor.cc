#include "lrit_processor.h"

#include <sys/stat.h>

#include "lrit/file.h"

#include "dir.h"

LRITProcessor::LRITProcessor(std::vector<std::unique_ptr<Handler> > handlers)
    : handlers_(std::move(handlers)) {
}

void LRITProcessor::run(int argc, char** argv) {
  std::vector<std::string> paths;

  // Gather file names from arguments (globs *.lrit* in directories).
  for (int i = 0; i < argc; i++) {
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

  // Process files in order
  for (const auto& path : paths) {
    auto file = std::make_shared<lrit::File>(path);
    for (auto& handler : handlers_) {
      handler->handle(file);
    }
  }
}
