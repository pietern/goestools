#pragma once

#include <string>
#include <vector>

struct Options {
  std::string nanomsg;
  std::vector<std::string> files;
  bool dryrun = false;
};

Options parseOptions(int argc, char** argv);
