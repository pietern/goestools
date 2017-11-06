#pragma once

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
  std::string channel;
  std::vector<LRIT::File> files;
  int fileType;
  bool shrink;
  std::string format;

  struct {
    int width;
    int height;
    CropWidth cropWidth = CropWidth::CENTER;
    CropHeight cropHeight = CropHeight::CENTER;
  } scale;
};

Options parseOptions(int argc, char** argv);
