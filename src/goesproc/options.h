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

  // Overwrite existing output files
  bool force = false;

  // Address of publisher to subscribe to (only relevant in packet mode)
  std::string subscribe;

  // Output directory
  std::string out = ".";

  // Paths specified as final argument(s)
  std::vector<std::string> paths;
};

Options parseOptions(int& argc, char**& argv);
