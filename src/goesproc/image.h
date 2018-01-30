#pragma once

#include <memory>
#include <string>

#include <opencv2/opencv.hpp>

#include "lrit/file.h"

#include "area.h"

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

  explicit Image(const lrit::File& f, cv::Mat m, const Area& area);

  void fillSides();

  void remap(const cv::Mat& mat);

  void save(const std::string& path) const;

  cv::Mat getRawImage() const;
  cv::Mat getRawImage(const Area& roi) const;

  cv::Mat getScaledImage(bool shrink) const;
  cv::Mat getScaledImage(const Area& roi, bool shrink) const;

protected:
  cv::Mat m_;

  // Measured relative to the offset in the ImageNavigationHeader
  Area area_;

  // Relative scaling of columns and lines.
  // This is applicable only for the GOES-N series.
  uint32_t columnScaling_;
  uint32_t lineScaling_;

private:
  cv::Size scaleSize(cv::Size s, bool shrink) const;
};
