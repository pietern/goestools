#pragma once

#include <chrono>
#include <string>

struct Options {
  std::string config;
  bool verbose;
  std::chrono::milliseconds interval;
};

Options parseOptions(int argc, char** argv);
