#include "filename.h"

#include <functional>
#include <stdexcept>

namespace {

using subst = std::function<std::string(const std::string&)>;

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

void replace(std::string& str, const std::string& keyword, subst fn) {
  std::string::size_type lpos = 0u;
  std::string::size_type rpos = 0u;
  const std::string prefix = "{" + keyword + ":";
  const std::string suffix = "}";
  while ((lpos = str.find(prefix, lpos)) != std::string::npos) {
    // Find corresponding suffix
    rpos = str.find(suffix, lpos + prefix.length());
    if (rpos == std::string::npos) {
      throw std::runtime_error("Invalid pattern");
    }

    // Lookup replacement
    auto klpos = lpos + prefix.length();
    auto krpos = rpos;
    const auto replace = fn(str.substr(klpos, krpos - klpos));

    // Replace
    str.replace(lpos, rpos - lpos + 1, replace);
    lpos += replace.length();
  }
}

void replace(std::string& str, const AWIPS& awips) {
  replace(str, "awips", [&] (const std::string& in) {
      if (in == "t1t2") {
        return awips.t1t2;
      } else if (in == "a1a2") {
        return awips.a1a2;
      } else if (in == "ii") {
        return awips.ii;
      } else if (in == "cccc") {
        return awips.cccc;
      } else if (in == "yy") {
        return awips.yy;
      } else if (in == "gggg") {
        return awips.gggg;
      } else if (in == "bbb") {
        return awips.bbb;
      } else if (in == "nnn") {
        return awips.nnn;
      } else if (in == "xxx") {
        return awips.xxx;
      }
      return std::string();
    });
}

void replace(std::string& str, const struct timespec& ts) {
  replace(str, "time", [&] (const std::string& in) {
      std::array<char, 128> tsbuf;
      auto len = strftime(
        tsbuf.data(),
        tsbuf.size(),
        in.c_str(),
        gmtime(&ts.tv_sec));
      return std::string(tsbuf.data(), len);
    });
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

  // Replace {time:FORMAT}
  replace(out, time);

  // Replace {awips:XXX}, if available
  if (awips != nullptr) {
    replace(out, *awips);
  }

  return out;
}
