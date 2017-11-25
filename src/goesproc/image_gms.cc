#include "image_gms.h"

ImageGMS::ImageGMS(LRIT::File file) : file_(file) {
  is_ = file_.getHeader<LRIT::ImageStructureHeader>();
  nl_ = file_.getHeader<LRIT::NOAALRITHeader>();
}

std::string ImageGMS::getSatellite() const {
  return "Himawari";
}

std::string ImageGMS::getProductShort() const {
  std::string product;
  if (nl_.productSubID % 2 == 1) {
    product = "FD";
  } else {
    std::array<char, 8> buf;
    auto len = snprintf(buf.data(), buf.size(), "S%05d", nl_.parameter);
    product = std::string(buf.data(), len);
  }
  return product;
}

std::string ImageGMS::getProductLong() const {
  std::string product;
  if (nl_.productSubID % 2 == 1) {
    product = "Full Disk";
  } else {
    product = "Sector ";
    std::array<char, 8> buf;
    auto num = nl_.parameter;
    auto len = snprintf(buf.data(), buf.size(), "%d", num);
    product += std::string(buf.data(), len);
  }
  return product;
}

std::string ImageGMS::getChannelShort() const {
  std::string channel;
  if (nl_.productSubID <= 2) {
    channel = "IR";
  } else {
    channel = "VS";
  }
  return channel;
}

std::string ImageGMS::getChannelLong() const {
  std::string channel;
  if (nl_.productSubID <= 2) {
    channel = "Infrared";
  } else {
    channel = "Visible";
  }
  return channel;
}

std::string ImageGMS::getTimeShort() const {
  std::array<char, 128> tsbuf;
  auto ts = file_.getHeader<LRIT::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y%m%d-%H%M%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::string ImageGMS::getTimeLong() const {
  std::array<char, 128> tsbuf;
  auto ts = file_.getHeader<LRIT::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y-%m-%d %H:%M:%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::string ImageGMS::getBasename() const {
  return
    getSatellite() + "_" +
    getProductShort() + "_" +
    getChannelShort() + "_" +
    getTimeShort();
}

cv::Mat ImageGMS::getRawImage() const {
  cv::Mat raw(is_.lines, is_.columns, CV_8UC1);
  auto ifs = file_.getData();
  ifs.read((char*)raw.data, is_.lines * is_.columns);
  assert(ifs);
  return raw;
}
