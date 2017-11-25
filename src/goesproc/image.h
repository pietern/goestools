#pragma once

#include <stdint.h>
#include <vector>

#include <opencv2/opencv.hpp>

#include "lib/file.h"

class Image {
public:
  Image(LRIT::File file);

  std::string getSatellite() const;
  std::string getProductShort() const;
  std::string getProductLong() const;
  std::string getChannelShort() const;
  std::string getChannelLong() const;
  std::string getTimeShort() const;
  std::string getTimeLong() const;
  std::string getBasename() const;

  cv::Mat getRawImage() const;

protected:
  LRIT::File file_;

  LRIT::ImageStructureHeader is_;
  LRIT::NOAALRITHeader nl_;
};
