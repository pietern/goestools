#include "image.h"

#include <memory>

#include "lib/util.h"

namespace {

// GOES 13 and 15 use identical region identification.
Image::Region getGOESLRITRegion(const LRIT::NOAALRITHeader& h) {
  Image::Region region;
  if (h.productSubID % 10 == 1) {
    region.nameShort = "FD";
    region.nameLong = "Full Disk";
  } else if (h.productSubID % 10 == 2) {
    region.nameShort = "NH";
    region.nameLong = "Northern Hemisphere";
  } else if (h.productSubID % 10 == 3) {
    region.nameShort = "SH";
    region.nameLong = "Southern Hemisphere";
  } else if (h.productSubID % 10 == 4) {
    region.nameShort = "US";
    region.nameLong = "United States";
  } else {
    std::array<char, 32> buf;
    size_t len;
    auto num = (h.productSubID % 10) - 5;
    len = snprintf(buf.data(), buf.size(), "SI%02d", num);
    region.nameShort = std::string(buf.data(), len);
    len = snprintf(buf.data(), buf.size(), "Special Interest %d", num);
    region.nameLong = std::string(buf.data(), len);
  }
  return region;
}

// GOES 13 and 15 use identical channel identification.
Image::Channel getGOESLRITChannel(const LRIT::NOAALRITHeader& h) {
  Image::Channel channel;
  if (h.productSubID <= 10) {
    channel.nameShort = "IR";
    channel.nameLong = "Infrared";
  } else if (h.productSubID <= 20) {
    channel.nameShort = "VS";
    channel.nameLong = "Visible";
  } else {
    channel.nameShort = "WV";
    channel.nameLong = "Water Vapor";
  }
  return channel;
}

} // namespace

std::unique_ptr<Image> Image::createFromFile(LRIT::File file) {
  auto nl = file.getHeader<LRIT::NOAALRITHeader>();
  switch (nl.productID) {
  case 13:
    return std::unique_ptr<Image>(new ImageGOES13(std::move(file)));
  case 15:
    return std::unique_ptr<Image>(new ImageGOES15(std::move(file)));
  case 16:
    return std::unique_ptr<Image>(new ImageGOES16(std::move(file)));
  case 43:
    return std::unique_ptr<Image>(new ImageHimawari8(std::move(file)));
  case 3:
    return std::unique_ptr<Image>(new ImageHimawari8(std::move(file)));
  case 4:
    return std::unique_ptr<Image>(new ImageMeteosat(std::move(file)));
  case 6:
    return std::unique_ptr<Image>(new ImageNWS(std::move(file)));
  }

  assert(false && "Unhandled productID");
}

Image::Image(LRIT::File file) : file_(file) {
  is_ = file_.getHeader<LRIT::ImageStructureHeader>();
  nl_ = file_.getHeader<LRIT::NOAALRITHeader>();
}

std::string Image::getSatellite() const {
  assert(false);
}

Image::Region Image::getRegion() const {
  assert(false);
}

Image::Channel Image::getChannel() const {
  assert(false);
}

std::string Image::getBasename() const {
  return
    getSatellite() + "_" +
    getRegionShort() + "_" +
    getChannelShort() + "_" +
    getTimeShort();
}

struct timespec Image::getTimeStamp() const {
  return file_.getHeader<LRIT::TimeStampHeader>().getUnix();
}

std::string Image::getTimeShort() const {
  std::array<char, 128> tsbuf;
  auto ts = getTimeStamp();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y%m%d-%H%M%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::string Image::getTimeLong() const {
  std::array<char, 128> tsbuf;
  auto ts = getTimeStamp();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y-%m-%d %H:%M:%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

cv::Mat Image::getRawImage() const {
  cv::Mat raw(is_.lines, is_.columns, CV_8UC1);
  auto ifs = file_.getData();

  if (is_.bitsPerPixel == 1) {
    auto ph = file_.getHeader<LRIT::PrimaryHeader>();

    // Number of pixels
    unsigned long n = (raw.size().width * raw.size().height);
    assert(ph.dataLength == n);

    // Read everything in one shot
    // Round up to nearest multiple of 8 because we're reading bytes
    auto line = std::make_unique<char[]>((n + 7) / 8);
    ifs->read(line.get(), (n + 7) / 8);
    assert(*ifs);

    // Pixel by pixel
    for (unsigned long i = 0; i < n; i += 8) {
      auto byte = line.get()[i / 8];
      for (auto j = i; j < (i + 8); j++) {
        if (byte & 0x80) {
          ((char*)raw.data)[j] = (char)0xff;
        } else {
          ((char*)raw.data)[j] = (char)0x00;
        }
        byte <<= 1;
      }
    }
  } else if (is_.bitsPerPixel == 8) {
    ifs->read((char*)raw.data, raw.size().width * raw.size().height);
    assert(ifs);
  } else {
    std::cerr << "bitsPerPixel == " << is_.bitsPerPixel << std::endl;
    assert(false);
  }
  return raw;
}

std::string ImageGOES13::getSatellite() const {
  return "GOES13";
}

Image::Region ImageGOES13::getRegion() const {
  return getGOESLRITRegion(this->nl_);
}

Image::Channel ImageGOES13::getChannel() const {
  return getGOESLRITChannel(this->nl_);
}

std::string ImageGOES15::getSatellite() const {
  return "GOES15";
}

Image::Region ImageGOES15::getRegion() const {
  return getGOESLRITRegion(this->nl_);
}

Image::Channel ImageGOES15::getChannel() const {
  return getGOESLRITChannel(this->nl_);
}

ImageGOES16::ImageGOES16(LRIT::File file) : Image(file) {
  auto text = getFile().getHeader<LRIT::AncillaryTextHeader>().text;
  auto pairs = split(text, ';');
  for (const auto& pair : pairs) {
    auto elements = split(pair, '=');
    assert(elements.size() == 2);
    auto key = trimRight(elements[0]);
    auto value = trimLeft(elements[1]);

    if (key == "Time of frame start") {
      auto ok = parseTime(value, &frameStart_);
      assert(ok);
      continue;
    }

    if (key == "Satellite") {
      satellite_ = value;
      continue;
    }

    if (key == "Instrument") {
      instrument_ = value;
      continue;
    }

    if (key == "Channel") {
      std::array<char, 32> buf;
      size_t len;
      auto num = std::stoi(value);
      assert(num >= 1 && num <= 16);
      len = snprintf(buf.data(), buf.size(), "CH%02d", num);
      channel_.nameShort = std::string(buf.data(), len);
      len = snprintf(buf.data(), buf.size(), "Channel %d", num);
      channel_.nameLong = std::string(buf.data(), len);
      continue;
    }

    if (key == "Imaging Mode") {
      imagingMode_ = value;
      continue;
    }

    if (key == "Region") {
      // Ignore; this needs to be derived from the file name
      // because the mesoscale sector is not included here.
      continue;
    }

    if (key == "Resolution") {
      resolution_ = value;
      continue;
    }

    if (key == "Segmented") {
      segmented_ = (value == "yes");
      continue;
    }

    std::cerr << "Unhandled key in ancillary text: " << key << std::endl;
    assert(false);
  }

  // Tell apart the two mesoscale sectors
  {
    auto fileName = getFile().getHeader<LRIT::AnnotationHeader>().text;
    auto parts = split(fileName, '-');
    assert(parts.size() >= 4);
    if (parts[2] == "CMIPF") {
      region_.nameLong = "Full Disk";
      region_.nameShort = "FD";
    } else if (parts[2] == "CMIPM1") {
      region_.nameLong = "Mesoscale 1";
      region_.nameShort = "M1";
    } else if (parts[2] == "CMIPM2") {
      region_.nameLong = "Mesoscale 2";
      region_.nameShort = "M2";
    } else {
      std::cerr << "Unable to derive region from: " << parts[2] << std::endl;
      assert(false);
    }
  }
}

std::string ImageGOES16::getSatellite() const {
  return "GOES16";
}

Image::Region ImageGOES16::getRegion() const {
  return region_;
}

Image::Channel ImageGOES16::getChannel() const {
  return channel_;
}

// Every GOES 16 ABI image has an ancillary text field
// that contains the exact time that the frame scan started.
struct timespec ImageGOES16::getTimeStamp() const {
  return frameStart_;
}

std::string ImageHimawari8::getSatellite() const {
  return "Himawari8";
}

Image::Region ImageHimawari8::getRegion() const {
  Region region;
  if (nl_.productSubID % 2 == 1) {
    region.nameShort = "FD";
    region.nameLong = "Full Disk";
  } else {
    assert(false);
  }
  return region;
}

Image::Channel ImageHimawari8::getChannel() const {
  Image::Channel channel;
  if (nl_.productSubID == 1) {
    channel.nameShort = "VS";
    channel.nameLong = "Visible";
  } else if (nl_.productSubID == 3) {
    channel.nameShort = "IR";
    channel.nameLong = "Infrared";
  } else if (nl_.productSubID == 7) {
    channel.nameShort = "WV";
    channel.nameLong = "Water Vapor";
  } else {
    assert(false);
  }
  return channel;
}

// Himawari 8 doesn't have a valid time stamp header
struct timespec ImageHimawari8::getTimeStamp() const {
  // Example annotation: IMG_DK01VIS_201712162250_003.lrit
  auto ah = file_.getHeader<LRIT::AnnotationHeader>();
  std::stringstream ss(ah.text);
  std::string tmp;
  assert(std::getline(ss, tmp, '_'));
  assert(std::getline(ss, tmp, '_'));
  assert(std::getline(ss, tmp, '_'));

  // Third substring represents time stamp
  struct timespec ts{0, 0};
  struct tm tm{0};
  tm.tm_year = std::stoi(tmp.substr(0, 4)) - 1900;
  tm.tm_mon = std::stoi(tmp.substr(4, 2)) - 1;
  tm.tm_mday = std::stoi(tmp.substr(6, 2));
  tm.tm_hour = std::stoi(tmp.substr(8, 2));
  tm.tm_min = std::stoi(tmp.substr(10, 2));
  ts.tv_sec = timegm(&tm);
  ts.tv_nsec = 0;

  return ts;
}

std::string ImageMeteosat::getSatellite() const {
  return "Meteosat";
}

Image::Region ImageMeteosat::getRegion() const {
  Image::Region region;
  if (nl_.productSubID % 2 == 1) {
    region.nameShort = "FD";
    region.nameLong = "Full Disk";
  } else {
    assert(false);
  }
  return region;
}

Image::Channel ImageMeteosat::getChannel() const {
  Image::Channel channel;
  if (nl_.productSubID <= 2) {
    channel.nameShort = "IR";
    channel.nameLong = "Infrared";
  } else if (nl_.productSubID <= 4) {
    channel.nameShort = "VS";
    channel.nameLong = "Visible";
 } else {
    channel.nameShort = "WV";
    channel.nameLong = "Water Vapor";
  }
  return channel;
}

std::string ImageNWS::getBasename() const {
  size_t pos;

  auto text = file_.getHeader<LRIT::AnnotationHeader>().text;

  // Use annotation without the "dat327221257926.lrit" suffix
  pos = text.find("dat");
  if (pos != std::string::npos) {
    text = text.substr(0, pos);
  }

  // Remove .lrit suffix
  pos = text.find(".lrit");
  if (pos != std::string::npos) {
    text = text.substr(0, pos);
  }

  // Add time if available
  if (file_.hasHeader<LRIT::TimeStampHeader>()) {
    text += "_" + getTimeShort();
  }

  return text;
}
