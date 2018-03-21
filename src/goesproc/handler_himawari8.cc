#include "handler_himawari8.h"

#include <cassert>

#include "lib/util.h"

#include "filename.h"
#include "string.h"

Himawari8ImageHandler::Himawari8ImageHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
  config_.region = toUpper(config_.region);
  for (auto& channel : config_.channels) {
    channel = toUpper(channel);
  }
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
  Region region;
  assert(nlh.productSubID % 2 == 1);
  region.nameShort = "FD";
  region.nameLong = "Full Disk";

  // Extract channel information
  Channel channel;
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
    return;
  }

  auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();
  auto imageIdentifier = getBasename(*f);
  auto& vector = segments_[imageIdentifier];
  vector.push_back(f);
  if (vector.size() == sih.maxSegment) {
    FilenameBuilder fb;
    fb.dir = config_.dir;
    fb.filename = getBasename(*f);
    fb.time = getTime(*f);
    fb.region = &region;
    fb.channel = &channel;

    auto path = fb.build(config_.filename, config_.format);
    auto image = Image::createFromFiles(vector);
    fileWriter_->write(path, image->getRawImage());

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

struct timespec Himawari8ImageHandler::getTime(const lrit::File& f) const {
  struct timespec time = {0, 0};

  // Time of Himawari images is encoded in filename.
  // The LRIT time stamp header is bogus.
  // Example annotation: IMG_DK01VIS_201712162250_003.lrit
  auto text = f.getHeader<lrit::AnnotationHeader>().text;
  auto parts = split(text, '_');
  if (parts.size() != 4) {
    return time;
  }

  const char* buf = parts[2].c_str();
  const char* format = "%Y%m%d%H%M";
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  auto ptr = strptime(buf, format, &tm);

  // Only use time if strptime was successful
  if (ptr == (buf + 12)) {
    time.tv_sec = mktime(&tm);
    time.tv_nsec = 0;
  }

  return time;
}
