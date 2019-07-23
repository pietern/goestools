#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

#include "area.h"
#include "gradient.h"

struct Config {
  struct Map {
    // Path to GeoJSON file
    std::string path;

    // Contents of GeoJSON file
    std::shared_ptr<const nlohmann::json> geo;

    // Line color
    cv::Scalar color;
  };

  struct Handler {
    // "image", "dcs", "text"
    std::string type;

    // "goes16", "himawari8", "nws", ...
    std::string origin;

    // For image handlers, this field is only used to filter GOES-R
    // ABI Level 2+ files. For non-GOES-R files, it is unused.
    //
    // Example: "cmip", "sst", "rrqpe", ...
    //
    std::vector<std::string> products;

    // "fd", "m1", "m2", "nh", "us", ...
    std::vector<std::string> regions;

    // "vs", "ir", "wv", "ch01", ...
    std::vector<std::string> channels;

    // Output directory
    std::string dir;

    // Output format ("png", "jpg", ...)
    std::string format;

    // Write LRIT header contents as JSON file.
    bool json = false;

    // Crop (applied before scaling)
    Area crop;

    // Remap is used to point handlers to a lookup table
    // to map raw values in an image to new values.
    // The lookup table must be loadable by OpenCV and must
    // have dimensions equal to either 1x256 or 256x1.
    std::map<std::string, cv::Mat> remap;

    // Gradient defines a parametric RGB or luminance curve
    // to be applied via the Image Data Function
    std::map<std::string, Gradient> gradient;
    enum GradientInterpolationType lerptype = LERP_UNDEFINED;

    // Lookup table to use to generate false color images
    cv::Mat lut;

    // Filename format (see filename.cc for more info)
    std::string filename;

    // Set of map overlays to apply
    std::vector<Map> maps;
  };

  static Config load(const std::string& file);

  explicit Config();

  bool ok;
  std::string error;

  std::vector<Handler> handlers;

  // Cache of JSON files to ensure the same file is never loaded twice.
  std::unordered_map<std::string, std::shared_ptr<const nlohmann::json>> json_;

  // Load JSON file at specified path.
  std::shared_ptr<const nlohmann::json> loadJSON(const std::string& path);
};
