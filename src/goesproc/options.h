#pragma once

#include <time.h>

#include <string>
#include <vector>

#include "lib/file.h"

enum class CropHeight {
  TOP,
  CENTER,
  BOTTOM,
};

enum class CropWidth {
  LEFT,
  CENTER,
  RIGHT,
};

struct Options {
  std::vector<std::string> channels;
  std::vector<LRIT::File> files;
  int fileType;
  bool shrink;
  std::string format;

  // Process files stamped between [start, stop)
  time_t start;
  time_t stop;

  struct {
    int width;
    int height;
    CropWidth cropWidth = CropWidth::CENTER;
    CropHeight cropHeight = CropHeight::CENTER;
  } scale;
};

Options parseOptions(int argc, char** argv);
