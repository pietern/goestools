#pragma once

#include <string>
#include <vector>

#include "file.h"

struct Options {
  std::string channel;
  std::vector<File> files;
  int fileType;
};

Options parseOptions(int argc, char** argv);
