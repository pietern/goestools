#include "image.h"

#include <memory>

Image::Image(LRIT::File file) : file_(file) {
  is_ = file_.getHeader<LRIT::ImageStructureHeader>();
  nl_ = file_.getHeader<LRIT::NOAALRITHeader>();
}

std::string Image::getSatellite() const {
  switch (nl_.productID) {
  case 13:
    return "GOES13";
  case 15:
    return "GOES15";
  case 3:
    return "Himawari";
  case 4:
    return "Meteosat";
  default:
    assert(false);
  }
  return "";
}

std::string Image::getProductShort() const {
  std::string product;
  switch (nl_.productID) {
  case 13:
  case 15:
    // GOES
    if (nl_.productSubID % 10 == 1) {
      product = "FD";
    } else if (nl_.productSubID % 10 == 2) {
      product = "NH";
    } else if (nl_.productSubID % 10 == 3) {
      product = "SH";
    } else if (nl_.productSubID % 10 == 4) {
      product = "US";
    } else {
      std::array<char, 8> buf;
      auto num = (nl_.productSubID % 10) - 5;
      auto len = snprintf(buf.data(), buf.size(), "SI%02d", num);
      product = std::string(buf.data(), len);
    }
    break;
  case 3:
  case 4:
    // Himawari/Meteosat
    if (nl_.productSubID % 2 == 1) {
      product = "FD";
    } else {
      std::array<char, 8> buf;
      auto len = snprintf(buf.data(), buf.size(), "S%05d", nl_.parameter);
      product = std::string(buf.data(), len);
    }
    break;
  default:
    assert(false);
  }
  return product;
}

std::string Image::getProductLong() const {
  std::string product;
  switch (nl_.productID) {
  case 13:
  case 15:
    // GOES
    if (nl_.productSubID % 10 == 1) {
      product = "Full Disk";
    } else if (nl_.productSubID % 10 == 2) {
      product = "Northern Hemisphere";
    } else if (nl_.productSubID % 10 == 3) {
      product = "Southern Hemisphere";
    } else if (nl_.productSubID % 10 == 4) {
      product = "United States";
    } else {
      product = "Special Interest ";
      std::array<char, 8> buf;
      auto num = (nl_.productSubID % 10) - 5;
      auto len = snprintf(buf.data(), buf.size(), "%d", num);
      product += std::string(buf.data(), len);
    }
    break;
  case 3:
  case 4:
    // Himawari/Meteosat
    if (nl_.productSubID % 2 == 1) {
      product = "Full Disk";
    } else {
      product = "Sector ";
      std::array<char, 8> buf;
      auto num = nl_.parameter;
      auto len = snprintf(buf.data(), buf.size(), "%d", num);
      product += std::string(buf.data(), len);
    }
    break;
  default:
    assert(false);
  }
  return product;
}

std::string Image::getChannelShort() const {
  std::string channel;
  switch (nl_.productID) {
  case 13:
  case 15:
    // GOES
    if (nl_.productSubID <= 10) {
      channel = "IR";
    } else if (nl_.productSubID <= 20) {
      channel = "VS";
    } else {
      channel = "WV";
    }
    break;
  case 3:
  case 4:
    // Himawari/Meteosat
    if (nl_.productSubID <= 2) {
      channel = "IR";
    } else if (nl_.productSubID <= 4) {
      channel = "VS";
    } else {
      channel = "WV";
    }
    break;
  default:
    assert(false);
  }
  return channel;
}

std::string Image::getChannelLong() const {
  std::string channel;
  switch (nl_.productID) {
  case 13:
  case 15:
    // GOES
    if (nl_.productSubID <= 10) {
      channel = "Infrared";
    } else if (nl_.productSubID <= 20) {
      channel = "Visible";
    } else {
      channel = "Water Vapor";
    }
    break;
  case 3:
  case 4:
    // Himawari/Meteosat
    if (nl_.productSubID <= 2) {
      channel = "Infrared";
    } else if (nl_.productSubID <= 4) {
      channel = "Visible";
    } else {
      channel = "Water Vapor";
    }
    break;
  default:
    assert(false);
  }
  return channel;
}

std::string Image::getTimeShort() const {
  std::array<char, 128> tsbuf;
  auto ts = file_.getHeader<LRIT::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y%m%d-%H%M%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::string Image::getTimeLong() const {
  std::array<char, 128> tsbuf;
  auto ts = file_.getHeader<LRIT::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y-%m-%d %H:%M:%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::string Image::getBasename() const {
  if (nl_.productID == 13 || nl_.productID == 15) {
    // GOES
    return
      getSatellite() + "_" +
      getProductShort() + "_" +
      getChannelShort() + "_" +
      getTimeShort();
  } else if (nl_.productID == 3) {
    // Himawari
    return
      getSatellite() + "_" +
      getProductShort() + "_" +
      getChannelShort() + "_" +
      getTimeShort();
  } else if (nl_.productID == 4) {
    // Meteosat
    return
      getSatellite() + "_" +
      getProductShort() + "_" +
      getChannelShort() + "_" +
      getTimeShort();
  } else if (nl_.productID == 6) {
    // NWS
    // Use annotation without the "dat327221257926.lrit" suffix
    auto text = file_.getHeader<LRIT::AnnotationHeader>().text;
    auto pos = text.find("dat");
    assert(pos != std::string::npos);
    return text.substr(0, pos) + "_" + getTimeShort();
  } else {
    assert(false);
  }
  return "";
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
    ifs.read(line.get(), (n + 7) / 8);
    assert(ifs);

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
    ifs.read((char*)raw.data, raw.size().width * raw.size().height);
    assert(ifs);
  } else {
    std::cerr << "bitsPerPixel == " << is_.bitsPerPixel << std::endl;
    assert(false);
  }
  return raw;
}
