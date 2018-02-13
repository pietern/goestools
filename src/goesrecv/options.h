#pragma once

#include <string>

struct Options {
  std::string config;
};

Options parseOptions(int argc, char** argv);
