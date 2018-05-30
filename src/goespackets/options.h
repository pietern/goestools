#pragma once

#include <set>
#include <string>
#include <vector>

struct Options {
  std::string subscribe;
  std::vector<std::string> publish;
  std::vector<std::string> files;

  // Record packets stream
  bool record = false;
  std::string filename = "./packets-%FT%H:%M:00.raw";

  // Filter these VCIDs (include everything if empty)
  std::set<int> vcids;
};

Options parseOptions(int argc, char** argv);
