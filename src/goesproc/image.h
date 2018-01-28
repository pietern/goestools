#pragma once

#include <memory>
#include <string>

#include <opencv2/opencv.hpp>

#include "lrit/file.h"

class Image {
public:
  struct Region {
    std::string nameShort;
    std::string nameLong;
  };

  struct Channel {
    std::string nameShort;
    std::string nameLong;
  };

  static std::unique_ptr<Image> createFromFile(
    std::shared_ptr<const lrit::File> f);

  static std::unique_ptr<Image> createFromFiles(
    std::vector<std::shared_ptr<const lrit::File> > fs);

  explicit Image(const lrit::File& f, cv::Mat m);

  void save(const std::string& path) const;

protected:
  cv::Mat m_;
};
