#pragma once

#include <stdint.h>
#include <vector>

#include <opencv2/opencv.hpp>

#include "lib/file.h"

class ImageGOES {
public:
  struct Area {
    int minColumn;
    int maxColumn;
    int minLine;
    int maxLine;

    int width() const {
      return maxColumn - minColumn;
    }

    int height() const {
      return maxLine - minLine;
    }

    Area getIntersection(const Area& other) const {
      Area result;
      result.minColumn = std::max(minColumn, other.minColumn);
      result.maxColumn = std::min(maxColumn, other.maxColumn);
      result.minLine = std::max(minLine, other.minLine);
      result.maxLine = std::min(maxLine, other.maxLine);
      return result;
    }

    Area getUnion(const Area& other) const {
      Area result;
      result.minColumn = std::min(minColumn, other.minColumn);
      result.maxColumn = std::max(maxColumn, other.maxColumn);
      result.minLine = std::min(minLine, other.minLine);
      result.maxLine = std::max(maxLine, other.maxLine);
      return result;
    }
  };

  ImageGOES(uint16_t imageIdentifier, std::vector<LRIT::File> files);

  const std::vector<LRIT::File>& getFiles() const {
    return files_;
  }

  uint8_t getExpectedNumberFiles() {
    return maxSegment_;
  }

  uint8_t getActualNumberFiles() {
    return files_.size();
  }

  Area getArea() const {
    return area_;
  }

  bool complete() const {
    return maxSegment_ == files_.size();
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
  std::vector<LRIT::File> files_;

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
