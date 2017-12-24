#pragma once

#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include "lib/file.h"

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

  static std::unique_ptr<Image> createFromFile(LRIT::File file);

  explicit Image(LRIT::File file);

  const LRIT::File& getFile() const {
    return file_;
  }

  virtual std::string getSatellite() const;
  virtual Region getRegion() const;
  virtual Channel getChannel() const;
  virtual std::string getBasename() const;
  virtual struct timespec getTimeStamp() const;

  std::string getRegionShort() const {
    return getRegion().nameShort;
  }

  std::string getRegionLong() const {
    return getRegion().nameLong;
  }

  std::string getChannelShort() const {
    return getChannel().nameShort;
  }

  std::string getChannelLong() const {
    return getChannel().nameLong;
  }

  std::string getTimeShort() const;
  std::string getTimeLong() const;
  cv::Mat getRawImage() const;

protected:
  LRIT::File file_;

  LRIT::ImageStructureHeader is_;
  LRIT::NOAALRITHeader nl_;
};

class ImageGOES13 : public Image {
public:
  explicit ImageGOES13(LRIT::File file)
    : Image(file) {
  }

  virtual std::string getSatellite() const override;
  virtual Region getRegion() const override;
  virtual Channel getChannel() const override;
};

class ImageGOES15 : public Image {
public:
  explicit ImageGOES15(LRIT::File file)
    : Image(file) {
  }

  virtual std::string getSatellite() const override;
  virtual Region getRegion() const override;
  virtual Channel getChannel() const override;
};

class ImageGOES16 : public Image {
public:
  explicit ImageGOES16(LRIT::File file)
    : Image(file) {
  }

  virtual std::string getSatellite() const override;
  virtual Region getRegion() const override;
  virtual Channel getChannel() const override;
};

class ImageHimawari8 : public Image {
public:
  explicit ImageHimawari8(LRIT::File file)
    : Image(file) {
  }

  virtual std::string getSatellite() const override;
  virtual Region getRegion() const override;
  virtual Channel getChannel() const override;
  virtual struct timespec getTimeStamp() const override;
};

class ImageMeteosat : public Image {
public:
  explicit ImageMeteosat(LRIT::File file)
    : Image(file) {
  }

  virtual std::string getSatellite() const override;
  virtual Region getRegion() const override;
  virtual Channel getChannel() const override;
};

class ImageNWS : public Image {
public:
  explicit ImageNWS(LRIT::File file)
    : Image(file) {
  }

  virtual std::string getBasename() const override;
};
