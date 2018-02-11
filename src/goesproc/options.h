#pragma once

#include <string>
#include <vector>

enum class ProcessMode {
  UNDEFINED,
  PACKET,
  LRIT,
};

struct Options {
  // Path to configuration file
  std::string config;

  // What to process (stream of VCDU packets or LRIT files)
  ProcessMode mode;
};

Options parseOptions(int& argc, char**& argv);
