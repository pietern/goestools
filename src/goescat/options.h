#pragma once

#include <set>
#include <string>
#include <vector>

struct Options {
  std::string publisher;
  std::vector<std::string> files;

  // Filter these VCIDs (include everything if empty)
  std::set<int> vcids;
};

Options parseOptions(int argc, char** argv);
