#pragma once

#include <string>
#include <vector>

struct Config {
  struct Handler {
    // "image", "dcs", "text"
    std::string type;

    // "goes16", "himawari8", "nws", ...
    std::string product;

    // "fd", "m1", "m2", "nh", "us", ...
    std::string region;

    // "vs", "ir", "wv", "ch01", ...
    std::string channel;

    // Output directory
    std::string dir;
  };

  static Config load(const std::string& file);

  explicit Config();

  bool ok;
  std::string error;

  std::vector<Handler> handlers;
};
