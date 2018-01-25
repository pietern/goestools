#pragma once

#include <stdint.h>
#include <vector>

#include "area.h"
#include "image.h"

class SegmentedImage {
public:
  SegmentedImage(
    uint16_t imageIdentifier,
    std::vector<std::unique_ptr<Image> >&& images);

  const std::vector<std::unique_ptr<Image> >& getImages() const {
    return images_;
  }

  const std::unique_ptr<Image>& getImage() const {
    return images_.front();
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
  std::string getRegionShort() const;
  std::string getRegionLong() const;
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
  std::vector<std::unique_ptr<Image>> images_;

  uint8_t maxSegment_;
  uint16_t columns_;
  uint16_t lines_;

  // Measured relative to the offset in the ImageNavigationHeader
  Area area_;

  lrit::ImageNavigationHeader inh_;
  lrit::NOAALRITHeader nlh_;

private:
  cv::Size scaleSize(cv::Size s, bool shrink) const;
};
