#pragma once

#include <string>
#include <vector>

enum class Mode { RAW, QBT, EMWIN };

struct Options {
  std::string nanomsg;
  std::vector<std::string> files;
  Mode mode = Mode::RAW;
  std::string out = ".";
};

Options parseOptions(int argc, char** argv);
