#pragma once

#include "image.h"
#include "types.h"

struct FilenameBuilder {
  std::string dir;
  std::string filename;

  struct timespec time;
  AWIPS awips;
  Product product;
  Region region;
  Channel channel;

  std::string build(
    const std::string& pattern,
    const std::string extension = std::string()) const;
};
