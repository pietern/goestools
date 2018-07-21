#pragma once

#include <map>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "area.h"
#include "gradient.h"

struct Config {
  struct Handler {
    // "image", "dcs", "text"
    std::string type;

    // "goes16", "himawari8", "nws", ...
    std::string product;

    // "fd", "m1", "m2", "nh", "us", ...
    std::vector<std::string> regions;

    // "vs", "ir", "wv", "ch01", ...
    std::vector<std::string> channels;

    // Output directory
    std::string dir;

    // Output format ("png", "jpg", ...)
    std::string format;

    // Crop (applied before scaling)
    Area crop;

    // Remap is used to point handlers to a lookup table
    // to map raw values in an image to new values.
    // The lookup table must be loadable by OpenCV and must
    // have dimensions equal to either 1x256 or 256x1.
    std::map<std::string, cv::Mat> remap;

    // Just like remap, but takes an RGB lookup table
    std::map<std::string, cv::Mat> remap_rgb;

    // Gradient defines a parametric RGB or luminance curve
    // to be applied via the Image Data Function
    std::map<std::string, Gradient> gradient;
    enum GradientInterpolationType lerptype = LERP_UNDEFINED;

    // Lookup table to use to generate false color images
    cv::Mat lut;

    // Filename format (see filename.cc for more info)
    std::string filename;
  };

  static Config load(const std::string& file);

  explicit Config();

  bool ok;
  std::string error;

  std::vector<Handler> handlers;
};
