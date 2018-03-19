#pragma once

#include "image.h"
#include "types.h"

struct FilenameBuilder {
  Image::Region region;
  Image::Channel channel;
  struct timespec time;
  const AWIPS* awips;

  std::string build(const std::string& pattern) const;
};
