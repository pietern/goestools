#pragma once

#include <string>

struct Options {
  // LRIT or HRIT
  std::string downlinkType;

  // Airspy or RTLSDR
  std::string device;
};

Options parseOptions(int argc, char** argv);
