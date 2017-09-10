#pragma once

#include <string>
#include <vector>

#include "file.h"

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
  std::vector<File> files;
  int fileType;
  bool shrink;

  struct {
    int width;
    int height;
    CropWidth cropWidth = CropWidth::CENTER;
    CropHeight cropHeight = CropHeight::CENTER;
  } scale;
};

Options parseOptions(int argc, char** argv);
