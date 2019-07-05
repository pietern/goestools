#include "time.h"

#include <array>
#include <cstring>
#include <util/error.h>

namespace util {

std::string stringTime() {
  struct timespec ts;
  auto rv = clock_gettime(CLOCK_REALTIME, &ts);
  ASSERT(rv >= 0);
  std::array<char, 128> tsbuf;
  auto len = strftime(
      tsbuf.data(), tsbuf.size(), "%Y-%m-%dT%H:%M:%S.", gmtime(&ts.tv_sec));
  len += snprintf(
      tsbuf.data() + len, tsbuf.size() - len, "%03ldZ", ts.tv_nsec / 1000000);
  ASSERT(len < tsbuf.size());
  return std::string(tsbuf.data(), len);
}

bool parseTime(const std::string& in, struct timespec* ts) {
  const char* buf = in.c_str();
  struct tm tm;

  // Initialize tm struct to 0s before using it, otherwise mktime will behave unpredictably - Kons
  memset(&tm, 0x0, sizeof(tm));

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

  // The resulting t should always be greater 0, otherwise there was a problem with date conversion
  ASSERT(t > 0);

  ts->tv_sec = t;
  ts->tv_nsec = tv_nsec;
  return true;
}

} // namespace util
