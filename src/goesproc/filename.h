#pragma once

#include "image.h"

struct FilenameBuilder {
  Image::Region region;
  Image::Channel channel;
  struct timespec time;

  std::string build(const std::string& pattern) const;
};
