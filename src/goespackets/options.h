#pragma once

#include <set>
#include <string>
#include <vector>

struct Options {
  std::string subscribe;
  std::vector<std::string> publish;
  std::vector<std::string> files;
  bool record = false;

  // Filter these VCIDs (include everything if empty)
  std::set<int> vcids;
};

Options parseOptions(int argc, char** argv);
