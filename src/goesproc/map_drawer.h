#pragma once

#include "config.h"
#include "lrit/lrit.h"
#include "proj.h"

class MapDrawer {
public:
  explicit MapDrawer(
    const Config::Handler* config,
    float longitude,
    lrit::ImageNavigationHeader inh);

  cv::Mat draw(cv::Mat& in);

protected:
  void generatePoints(
    const Config::Map& map,
    std::vector<std::vector<cv::Point>>& out);

  void generatePoints(
    std::vector<std::vector<cv::Point>>& out,
    const nlohmann::json& poly);

  const Config::Handler* config_;
  Proj proj_;
  lrit::ImageNavigationHeader inh_;

  // Store one vector of line segments per map in the handler configuration.
  std::vector<std::vector<std::vector<cv::Point>>> points_;
};
