#include "util.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <array>
#include <algorithm>
#include <cassert>
#include <cstring>

namespace {

const std::string whitespace = " \f\n\r\t\v";

} // namespace

std::vector<std::string> split(std::string in, char delim) {
  std::vector<std::string> items;
  std::istringstream ss(in);
  std::string item;
  while (std::getline(ss, item, delim)) {
    items.push_back(item);
  }
  return items;
}

std::string trimLeft(const std::string& in) {
  return in.substr(in.find_first_not_of(whitespace));
}

std::string trimRight(const std::string& in) {
  return in.substr(0, in.find_last_not_of(whitespace) + 1);
}

std::string stringTime() {
  struct timespec ts;
  auto rv = clock_gettime(CLOCK_REALTIME, &ts);
  assert(rv >= 0);
  std::array<char, 128> tsbuf;
  auto len = strftime(tsbuf.data(), tsbuf.size(), "%Y-%m-%dT%H:%M:%S.", gmtime(&ts.tv_sec));
  len += snprintf(tsbuf.data() + len, tsbuf.size() - len, "%03ldZ", ts.tv_nsec / 1000000);
  assert(len < tsbuf.size());
  return std::string(tsbuf.data(), len);
}

bool parseTime(const std::string& in, struct timespec* ts) {
  const char* buf = in.c_str();
  struct tm tm;
  long int tv_nsec = 0;

  // For example: 2017-12-21T17:46:32.2Z
  char* pos = strptime(buf, "%Y-%m-%dT%H:%M:%S", &tm);
  if (pos < (buf + in.size())) {
    if (pos[0] == '.') {
      // Expect single decimal for fractional second
      int dec = 0;
      int num = sscanf(pos, ".%dZ", &dec);
      if (num == 1 && dec < 10) {
        ts->tv_nsec = num * 100000000;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  auto t = mktime(&tm);
  ts->tv_sec = t;
  ts->tv_nsec = tv_nsec;
  return true;
}

void mkdirp(const std::string& path) {
  size_t pos = 0;

  for (;; pos++) {
    pos = path.find('/', pos);
    if (pos == 0) {
      continue;
    }
    auto sub = path.substr(0, pos);
    auto rv = mkdir(sub.c_str(), S_IRWXU);
    if (rv == -1 && errno != EEXIST) {
      perror("mkdir");
      assert(rv == 0);
    }
    if (pos == std::string::npos) {
      break;
    }
  }
}

std::string toLower(const std::string& in) {
  std::string out;
  out.resize(in.size());
  std::transform(in.begin(), in.end(), out.begin(), ::tolower);
  return out;
}

std::string toUpper(const std::string& in) {
  std::string out;
  out.resize(in.size());
  std::transform(in.begin(), in.end(), out.begin(), ::toupper);
  return out;
}

size_t findLast(const std::string& in, char c) {
  auto pos = in.find('_');
  while (pos != std::string::npos) {
    auto npos = in.find(c, pos + 1);
    if (npos == std::string::npos) {
      break;
    }
    pos = npos;
  }
  return pos;
}
