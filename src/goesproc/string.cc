#include "string.h"

std::string removeSuffix(const std::string& str) {
  auto pos = str.rfind('.');
  if (pos != std::string::npos) {
    return str.substr(0, pos);
  }
  return str;
}
