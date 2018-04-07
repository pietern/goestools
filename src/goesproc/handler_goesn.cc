#include "handler_goesn.h"

#include <cassert>

#include "lib/util.h"

#include "filename.h"
#include "string.h"

namespace {

// The "Time of frame start" pair in the ancillary header uses
// the day of the year instead of year/month/day.
// This is not used elsewhere so the parsing code can stay local.
bool goesnParseTime(const std::string& in, struct timespec* ts) {
  const char* buf = in.c_str();
  struct tm tm;

  // For example: 2018/079/00:00:18
  char* pos = strptime(buf, "%Y/%j/%H:%M:%S", &tm);
  if (pos < (buf + in.size())) {
    return false;
  }

  auto t = mktime(&tm);
  ts->tv_sec = t;
  ts->tv_nsec = 0;
  return true;
}

} // namespace

GOESNImageHandler::GOESNImageHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
  for (auto& region : config_.regions) {
    region = toUpper(region);
  }
  for (auto& channel : config_.channels) {
    channel = toUpper(channel);
  }

  if (config_.product == "goes13") {
    productID_ = 13;
  } else if (config_.product == "goes15") {
    productID_ = 15;
  } else {
    assert(false);
  }
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
  if (!config_.regions.empty()) {
    auto begin = std::begin(config_.regions);
    auto end = std::end(config_.regions);
    auto it = std::find(begin, end, region.nameShort);
    if (it == end) {
      return;
    }
  }

  // Filter by channel
  auto channel = loadChannel(nlh);
  if (!config_.channels.empty()) {
    auto begin = std::begin(config_.channels);
    auto end = std::end(config_.channels);
    auto it = std::find(begin, end, channel.nameShort);
    if (it == end) {
      return;
    }
  }

  // Assume all images are segmented
  if (!f->hasHeader<lrit::SegmentIdentificationHeader>()) {
    return;
  }

  auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();
  auto key = std::make_pair(region.nameShort, channel.nameShort);
  auto& vector = segments_[key];

  // Ensure we can append this segment
  if (!vector.empty()) {
    const auto& t = vector.front();
    auto tsih = t->getHeader<lrit::SegmentIdentificationHeader>();
    if (tsih.imageIdentifier != sih.imageIdentifier) {
      vector.clear();
    }
  }

  vector.push_back(f);
  if (vector.size() == sih.maxSegment) {
    auto first = vector.front();
    auto image = Image::createFromFiles(vector);
    vector.clear();

    auto text = first->getHeader<lrit::AnnotationHeader>().text;
    auto filename = removeSuffix(text);
    auto details = loadDetails(*first);

    FilenameBuilder fb;
    fb.dir = config_.dir;
    fb.filename = filename;
    fb.time = details.frameStart;
    fb.region = &region;
    fb.channel = &channel;

    cv::Mat raw;
    if (config_.crop.empty()) {
      raw = image->getScaledImage(false);
    } else {
      raw = image->getScaledImage(config_.crop, false);
    }

    auto path = fb.build(config_.filename, config_.format);
    fileWriter_->write(path, raw);
    return;
  }
}

GOESNImageHandler::Details GOESNImageHandler::loadDetails(
    const lrit::File& f) {
  GOESNImageHandler::Details details;

  auto text = f.getHeader<lrit::AncillaryTextHeader>().text;
  auto pairs = split(text, ';');
  for (const auto& pair : pairs) {
    auto elements = split(pair, '=');
    assert(elements.size() == 2);
    auto key = trimRight(elements[0]);
    auto value = trimLeft(elements[1]);

    if (key == "Time of frame start") {
      auto ok = goesnParseTime(value, &details.frameStart);
      assert(ok);
      continue;
    }

    if (key == "Satellite") {
      details.satellite = value;
      continue;
    }
  }

  return details;
}

Region GOESNImageHandler::loadRegion(const lrit::NOAALRITHeader& h) const {
  Region region;
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

Channel GOESNImageHandler::loadChannel(const lrit::NOAALRITHeader& h) const {
  Channel channel;
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
