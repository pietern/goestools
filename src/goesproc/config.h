#pragma once

#include <string>
#include <vector>

#include "area.h"

struct Config {
  struct Handler {
    // "image", "dcs", "text"
    std::string type;

    // "goes16", "himawari8", "nws", ...
    std::string product;

    // "fd", "m1", "m2", "nh", "us", ...
    std::string region;

    // "vs", "ir", "wv", "ch01", ...
    std::vector<std::string> channels;

    // Output directory
    std::string dir;

    // Crop (applied before scaling)
    Area crop;
  };

  static Config load(const std::string& file);

  explicit Config();

  bool ok;
  std::string error;

  std::vector<Handler> handlers;
};
