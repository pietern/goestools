#pragma once

#include "image.h"
#include "types.h"

struct FilenameBuilder {
  std::string dir;
  std::string filename;

  Image::Region region;
  Image::Channel channel;
  struct timespec time;
  const AWIPS* awips;

  std::string build(
    const std::string& pattern,
    const std::string extension = std::string()) const;
};
