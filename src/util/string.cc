#include "string.h"

#include <algorithm>
#include <sstream>

namespace {

const std::string whitespace = " \f\n\r\t\v";

} // namespace

namespace util {

std::vector<std::string> split(std::string in, char delim) {
  std::vector<std::string> items;
  std::istringstream ss(in);
  std::string item;
  while (std::getline(ss, item, delim)) {
    items.push_back(item);
  }
  return items;
}

std::string join(const std::vector<std::string>& in, char delim) {
  std::stringstream ss;
  for (size_t i = 0; i < in.size(); i++) {
    if (i > 0) {
      ss << delim;
    }
    ss << in[i];
  }
  return ss.str();
}

std::string trimLeft(const std::string& in) {
  return in.substr(in.find_first_not_of(whitespace));
}

std::string trimRight(const std::string& in) {
  return in.substr(0, in.find_last_not_of(whitespace) + 1);
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

} // namespace util
