#include "handler_himawari8.h"

#include <cassert>

#include "lib/util.h"

Himawari8ImageHandler::Himawari8ImageHandler(const Config::Handler& config)
    : config_(config) {
  config_.region = toUpper(config_.region);
  for (auto& channel : config_.channels) {
    channel = toUpper(channel);
  }

  // Ensure output directory exists
  mkdirp(config_.dir);
}

void Himawari8ImageHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 0) {
    return;
  }

  // Filter Himawari 8
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID != 43) {
    return;
  }

  // All images relayed from Himawari 8 are segmented.
  // I have seen files without segment identification header
  // and that have fewer bytes than listed in the header.
  // I think they are corrupted, so let's ignore them.
  if (!f->hasHeader<lrit::SegmentIdentificationHeader>()) {
    return;
  }

  // Expect only full disk images
  Image::Region region;
  assert(nlh.productSubID % 2 == 1);
  region.nameShort = "FD";
  region.nameLong = "Full Disk";

  Image::Channel channel;
  if (nlh.productSubID == 1) {
    channel.nameShort = "VS";
    channel.nameLong = "Visible";
  } else if (nlh.productSubID == 3) {
    channel.nameShort = "IR";
    channel.nameLong = "Infrared";
  } else if (nlh.productSubID == 7) {
    channel.nameShort = "WV";
    channel.nameLong = "Water Vapor";
  } else {
    assert(false);
  }

  assert(f->hasHeader<lrit::SegmentIdentificationHeader>());
  auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();
  auto imageIdentifier = getBasename(*f);
  auto& vector = segments_[imageIdentifier];
  vector.push_back(f);
  if (vector.size() == sih.maxSegment) {
    auto image = Image::createFromFiles(vector);
    image->save(config_.dir + "/" + imageIdentifier + "." + config_.format);

    // Remove from handler cache
    segments_.erase(imageIdentifier);
    return;
  }
}

std::string Himawari8ImageHandler::getBasename(const lrit::File& f) const {
  auto text = f.getHeader<lrit::AnnotationHeader>().text;

  // Find last _ in annotation text and use this prefix
  // as identification for this segmented image.
  // Example annotation: IMG_DK01VIS_201712162250_003.lrit
  auto pos = findLast(text, '_');
  assert(pos != std::string::npos);
  return text.substr(0, pos);
}
