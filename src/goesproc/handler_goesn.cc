#include "handler_goesn.h"

#include <util/error.h>
#include <util/string.h>

#include "lib/timer.h"

#include "filename.h"
#include "string.h"

#ifdef HAS_PROJ
#include "map_drawer.h"
#endif

using namespace util;

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

// Insert segment in vector such that the segments remain ordered.
void insertSegment(
    std::vector<std::shared_ptr<const lrit::File>>& files,
    std::shared_ptr<const lrit::File> file) {
  auto sih = file->getHeader<lrit::SegmentIdentificationHeader>();
  auto pos = std::find_if(files.begin(), files.end(), [&sih] (auto& tf) {
      auto tsih = tf->template getHeader<lrit::SegmentIdentificationHeader>();
      return sih.segmentNumber < tsih.segmentNumber;
    });
  files.insert(pos, file);
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

  if (config_.origin == "goes13") {
    productID_ = 13;
  } else if (config_.origin == "goes15") {
    productID_ = 15;
  } else {
    ASSERT(false);
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
  auto key = std::make_tuple("unused", region.nameShort, channel.nameShort);
  auto& vector = segments_[key];

  // Ensure we can append this segment
  if (!vector.empty()) {
    const auto& t = vector.front();

    // Use "Time of frame start" field in ancillary text header to
    // check that this segment belongs to the set of existing ones.
    // Previously this was done with the segment identification
    // header, but a ground station update in August 2018 introduced
    // an error where segments of the same image often don't share the
    // same image identification header. This is only the case for the
    // GOES-16 (possibly also GOES-17) relay of GOES-15 on HRIT.
    auto da = loadDetails(*f);
    auto db = loadDetails(*t);
    if (da.frameStart.tv_sec != db.frameStart.tv_sec) {
      vector.clear();
    }
  }

  insertSegment(vector, f);
  if (vector.size() == sih.maxSegment) {
    Timer t;

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
    fb.region = region;
    fb.channel = channel;

    cv::Mat raw;
    if (config_.crop.empty()) {
      raw = image->getScaledImage(false);
    } else {
      raw = image->getScaledImage(config_.crop, false);
    }

    overlayMaps(*first, config_.crop, raw);
    auto path = fb.build(config_.filename, config_.format);
    fileWriter_->write(path, raw, &t);
    if (config_.json) {
      fileWriter_->writeHeader(*first, path);
    }
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
    ASSERT(elements.size() == 2);
    auto key = trimRight(elements[0]);
    auto value = trimLeft(elements[1]);

    if (key == "Time of frame start") {
      auto ok = goesnParseTime(value, &details.frameStart);
      ASSERT(ok);
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

void GOESNImageHandler::overlayMaps(
    const lrit::File& f,
    const Area& crop,
    cv::Mat& mat) {
#ifdef HAS_PROJ
  if (config_.maps.empty()) {
    return;
  }

  auto inh = f.getHeader<lrit::ImageNavigationHeader>();
  auto lon = inh.getLongitude();

  // GOES-16 doing relay of GOES-15 reports -135.5 but is actually at -135.0
  if (fabsf(lon - (-135.5)) < 1e-3) {
    lon = -135.0;
  }

  // GOES-15 drift completed on November 7, 2018 at 1900 UTC.
  // GOES-16 and GOES-17 are reporting its previous location
  // in the headers of the relayed LRITs.
  auto ts = f.getHeader<lrit::TimeStampHeader>().getUnix();
  if ((fabsf(lon - (-135.0)) < 1e-3) && ts.tv_sec >= 1541617200) {
    lon = -128.0;
  }

  // If a crop was used, the column and line offsets need to be fixed.
  if (!crop.empty()) {
    inh.columnOffset -= (crop.minColumn - -((int32_t) inh.columnOffset));
    inh.lineOffset -= (crop.minLine - -(int32_t) inh.lineOffset);
  }

  // The image already has the aspect ratio fixed; do it here as well.
  inh.lineOffset *= ((float) inh.columnScaling / (float) inh.lineScaling);
  inh.lineScaling = inh.columnScaling;

  // Smudge factor to make it visually match up better
  inh.columnScaling *= 1.001;
  inh.lineScaling *= 1.001;

  // TODO: The map drawer should be cached by construction parameters.
  auto drawer = MapDrawer(&config_, lon, inh);
  mat = drawer.draw(mat);
#endif
}
