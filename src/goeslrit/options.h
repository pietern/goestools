#pragma once

#include <set>
#include <string>
#include <vector>

struct Options {
  std::string nanomsg;
  std::vector<std::string> files;
  std::set<int> vcids;
  bool dryrun = false;
  std::string out = ".";

  // File types to include
  bool images = false;
  bool messages = false;
  bool text = false;
  bool dcs = false;
  bool emwin = false;
};

Options parseOptions(int argc, char** argv);
