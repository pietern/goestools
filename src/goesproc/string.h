#pragma once

#include <string>

// Find last occurrence of `c` in `in`.
inline size_t findLast(const std::string& in, char c) {
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
