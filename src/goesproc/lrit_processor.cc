#include "lrit_processor.h"

#include <sys/stat.h>

#include <algorithm>

#include "lib/dir.h"
#include "lrit/file.h"

LRITProcessor::LRITProcessor(std::vector<std::unique_ptr<Handler> > handlers)
    : handlers_(std::move(handlers)) {
}

void LRITProcessor::run(int argc, char** argv) {
  std::vector<std::shared_ptr<lrit::File>> files;
  std::map<std::string, int64_t> time;

  // Gather files from arguments (globs *.lrit* in directories).
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
      for (const auto& path : result) {
        files.push_back(std::make_shared<lrit::File>(path));
      }
    } else {
      files.push_back(std::make_shared<lrit::File>(argv[i]));
    }
  }

  // Gather time stamps (per-second granularity is plenty)
  for (const auto& file : files) {
    if (file->hasHeader<lrit::TimeStampHeader>()) {
      auto ts = file->getHeader<lrit::TimeStampHeader>().getUnix();
      time[file->getName()] = ts.tv_sec;
    } else {
      time[file->getName()] = 0;
    }
  }

  // Sort files by their LRIT time
  std::sort(
    files.begin(),
    files.end(),
    [&time](const auto& a, const auto& b) -> bool {
      return time[a->getName()] < time[b->getName()];
    });

  // Process files in chronological order
  for (const auto& file : files) {
    for (auto& handler : handlers_) {
      handler->handle(file);
    }
  }
}
