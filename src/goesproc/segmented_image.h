#pragma once

#include <stdint.h>
#include <vector>

#include "area.h"
#include "image.h"

class SegmentedImage {
public:
  SegmentedImage(uint16_t imageIdentifier, std::vector<Image> images);

  const std::vector<Image>& getImages() {
    return images_;
  }

  uint8_t getExpectedNumberFiles() {
    return maxSegment_;
  }

  uint8_t getActualNumberFiles() {
    return images_.size();
  }

  Area getArea() const {
    return area_;
  }

  bool complete() const {
    return maxSegment_ == images_.size();
  }

  std::string getSatellite() const;
  std::string getProductShort() const;
  std::string getProductLong() const;
  std::string getChannelShort() const;
  std::string getChannelLong() const;
  std::string getTimeShort() const;
  std::string getTimeLong() const;
  std::string getBasename() const;

  cv::Mat getRawImage() const;
  cv::Mat getRawImage(const Area& roi) const;

  cv::Mat getScaledImage(bool shrink) const;
  cv::Mat getScaledImage(const Area& roi, bool shrink) const;

protected:
  uint16_t imageIdentifier_;
  std::vector<Image> images_;

  uint8_t maxSegment_;
  uint16_t columns_;
  uint16_t lines_;

  // Measured relative to the offset in the ImageNavigationHeader
  Area area_;

  LRIT::ImageNavigationHeader inh_;
  LRIT::NOAALRITHeader nlh_;

private:
  cv::Size scaleSize(cv::Size s, bool shrink) const;
};
