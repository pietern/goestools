#include "filename.h"

namespace {

void replace(
    std::string& str,
    const std::string& find,
    const std::string& replace) {
  std::string::size_type pos = 0u;
  while ((pos = str.find(find, pos)) != std::string::npos) {
    str.replace(pos, find.length(), replace);
    pos += replace.length();
  }
}

std::string toISO8601(struct timespec ts) {
  std::array<char, 128> tsbuf;
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y%m%d-%H%M%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

} // namespace

std::string FilenameBuilder::build(const std::string& pattern) const {
  std::string out = pattern;

  // %r: Region
  replace(out, "%r", region.nameShort);

  // %c: Channel
  replace(out, "%c", channel.nameShort);

  // %t: Date and time (ISO 8601)
  replace(out, "%t", toISO8601(time));

  return out;
}
