#include "dir.h"

#include <util/error.h>

Dir::Dir(const std::string& path)
    : path_(path) {
  dir_ = opendir(path.c_str());
  ASSERT(dir_ != nullptr);
}

Dir::~Dir() {
  closedir(dir_);
}

std::vector<std::string> Dir::matchFiles(const std::string& pattern) {
  std::vector<std::string> result;
  struct dirent *ent;
  while ((ent = readdir(dir_)) != NULL) {
    if (fnmatch(pattern.c_str(), ent->d_name, 0) == 0) {
      result.push_back(path_ + "/" + std::string(ent->d_name));
    }
  }
  return result;
}
