#include "handler_goesn.h"

#include <cassert>

#include "lib/util.h"

GOESNImageHandler::GOESNImageHandler(const Config::Handler& config)
    : config_(config) {
  config_.region = toUpper(config_.region);
  config_.channel = toUpper(config_.channel);

  if (config_.product == "goes13") {
    productID_ = 13;
  } else if (config_.product == "goes15") {
    productID_ = 15;
  } else {
    assert(false);
  }

  // Ensure output directory exists
  mkdirp(config_.dir);
}

void GOESNImageHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 0) {
    return;
  }

  // Filter by product
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID != productID_) {
    return;
  }

  // Filter by region
  auto region = loadRegion(nlh);
  if (config_.region != region.nameShort) {
    return;
  }

  // Filter by channel
  auto channel = loadChannel(nlh);
  if (!config_.channel.empty() && config_.channel != channel.nameShort) {
    return;
  }

  if (!f->hasHeader<lrit::SegmentIdentificationHeader>()) {
    assert("TODO");
  }

  assert(f->hasHeader<lrit::SegmentIdentificationHeader>());
  auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();
  auto& map = segments_[channel.nameShort];
  auto& vector = map[sih.imageIdentifier];
  vector.push_back(f);
  if (vector.size() == sih.maxSegment) {
    auto image = Image::createFromFiles(vector);
    auto filename = getBasename(*f);

    cv::Mat raw;
    if (config_.crop.empty()) {
      raw = image->getScaledImage(false);
    } else {
      raw = image->getScaledImage(config_.crop, false);
    }

    cv::imwrite(config_.dir + "/" + filename + ".png", raw);

    // Remove from handler cache
    map.erase(sih.imageIdentifier);
    return;
  }
}

std::string GOESNImageHandler::getBasename(const lrit::File& f) const {
  size_t pos;

  auto text = f.getHeader<lrit::AnnotationHeader>().text;

  // Remove .lrit suffix
  pos = text.find(".lrit");
  if (pos != std::string::npos) {
    text = text.substr(0, pos);
  }

  return text;
}

Image::Region GOESNImageHandler::loadRegion(const lrit::NOAALRITHeader& h) const {
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

Image::Channel GOESNImageHandler::loadChannel(const lrit::NOAALRITHeader& h) const {
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
