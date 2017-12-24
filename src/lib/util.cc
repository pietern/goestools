#include "util.h"

std::vector<std::string> split(std::string in, char delim) {
  std::vector<std::string> items;
  std::istringstream ss(in);
  std::string item;
  while (std::getline(ss, item, delim)) {
    items.push_back(item);
  }
  return items;
}
