#include "fs.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <util/error.h>

namespace util {

void mkdirp(const std::string& path) {
  size_t pos = 0;
  for (;; pos++) {
    pos = path.find('/', pos);
    if (pos == 0) {
      continue;
    }
    auto sub = path.substr(0, pos);
    constexpr auto mode =
        S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    auto rv = mkdir(sub.c_str(), mode);
    if (rv == -1 && errno != EEXIST) {
      perror("mkdir");
      ASSERT(rv == 0);
    }
    if (pos == std::string::npos) {
      break;
    }
  }
}

} // namespace util
