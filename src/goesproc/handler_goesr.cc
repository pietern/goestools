#include "handler_goesr.h"

#include <stdexcept>

#include <util/error.h>
#include <util/string.h>
#include <util/time.h>

#include "lib/timer.h"

#include "string.h"

#ifdef HAS_PROJ
#include "map_drawer.h"
#endif

using namespace util;

namespace {

int getChannelFromFileName(const std::string& fileName) {
  const auto parts = split(fileName, '-');
  ASSERT(parts.size() >= 4);
  int mode = -1;
  int channel = -1;
  auto rv = sscanf(parts[3].c_str(), "M%dC%02d", &mode, &channel);
  if (rv == 2) {
    return channel;
  }
  return -1;
}

} // namespace

GOESRProduct::GOESRProduct(const std::shared_ptr<const lrit::File>& f)
    : files_({f}) {
  const auto fileName = f->getHeader<lrit::AnnotationHeader>().text;
  const auto fileNameParts = split(fileName, '-');
  ASSERT(fileNameParts.size() >= 4);
  auto text = f->getHeader<lrit::AncillaryTextHeader>().text;
  auto pairs = split(text, ';');
  for (const auto& pair : pairs) {
    auto elements = split(pair, '=');
    ASSERT(elements.size() == 2);
    auto key = trimRight(elements[0]);
    auto value = trimLeft(elements[1]);

    if (key == "Time of frame start") {
      auto ok = parseTime(value, &frameStart_);
      ASSERT(ok);
      continue;
    }

    if (key == "Satellite") {
      satellite_ = value;

      // First skip over non-digits.
      // Expect a value of "G16" or "G17".
      std::stringstream ss(value);
      while (!isdigit(ss.peek())) {
        ss.get();
      }
      ss >> satelliteID_;
      continue;
    }

    if (key == "Instrument") {
      instrument_ = value;
      continue;
    }

    if (key == "Channel") {
      std::array<char, 32> buf;
      size_t len;
      int num = -1;
      // The ancillary text header may include the integer channel number.
      // If it doesn't (e.g. it is "N/A"), extract it from the filename.
      // See https://github.com/pietern/goestools/issues/48 for an example
      // situation where we had to fall back to extracting the channel
      // number from the file name for perfectly valid CMIP files.
      auto rv = sscanf(value.c_str(), "%d", &num);
      if (rv != 1) {
        num = getChannelFromFileName(fileName);
      }
      // Populate channel number only if it is valid.
      if (num > 0) {
        ASSERTM(num >= 1 && num <= 16, "num = ", num);
        len = snprintf(buf.data(), buf.size(), "CH%02d", num);
        channel_.nameShort = std::string(buf.data(), len);
        len = snprintf(buf.data(), buf.size(), "Channel %d", num);
        channel_.nameLong = std::string(buf.data(), len);
      }
      continue;
    }

    if (key == "Imaging Mode") {
      imagingMode_ = value;
      continue;
    }

    if (key == "Region") {
      if (value == "Full Disk") {
        region_.nameLong = "Full Disk";
        region_.nameShort = "FD";
      } else if (value == "Mesoscale") {
        // The mesoscale sector number is not included in the ancillary
        // text header. If this is a CMIP file, we know which chunk of the
        // file name to check to figure out the mesoscale region. We don't
        // know if there will ever be non-CMIP mesoscale images.
        if (fileNameParts[2] == "CMIPM1") {
          region_.nameLong = "Mesoscale 1";
          region_.nameShort = "M1";
        } else if (fileNameParts[2] == "CMIPM2") {
          region_.nameLong = "Mesoscale 2";
          region_.nameShort = "M2";
        } else {
          FAILM(
              "Unable to derive product region from value \"",
              fileNameParts[2],
              "\"");
        }
      } else {
        FAILM("Unable to derive product region from value \"", value, "\"");
      }
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

    // New keys were added to this map which tripped this assert.
    // Started happening on Apr 2, 2020. If these contain useful
    // information, we can choose to properly process them at a
    // later point in time. Until then, ignore them.
    //
    //   FAILM("Unhandled key in ancillary text \"", key, "\"");
    //
  }

  // The product name is encoded in the file name only.
  // For example: OR_ABI-L2-ACHTF-M6_G16_[...].lrit.
  // The samples I've seen all have a suffix equal to the
  // short hand of the region, e.g. "F" or "M1".
  {
    const auto tmp = fileNameParts[2];
    const auto s1 = tmp.substr(tmp.size() - 1);
    const auto s2 = tmp.substr(tmp.size() - 2);
    if (s1 == "F") {
      product_.nameShort = tmp.substr(0, tmp.size() - s1.size());
    } else if (s2 == "M1" || s2 == "M2") {
      product_.nameShort = tmp.substr(0, tmp.size() - s2.size());
    } else {
      product_.nameShort = tmp;
    }

    // We can fix a mapping from the abbreviation to the long name
    // for all the products, but for now, make them equivalent.
    product_.nameLong = product_.nameShort;
  }
}

// Add file to list of segments such that the list remains sorted by
// segment number. There are no guarantees that segment files are
// transmitted in the order they should be processed in, so we must
// take care they are re-ordered here.
void GOESRProduct::add(const std::shared_ptr<const lrit::File>& f) {
  auto s = f->getHeader<lrit::SegmentIdentificationHeader>();
  auto pos = std::find_if(files_.begin(), files_.end(), [&s](auto tf) {
    auto ts = tf->template getHeader<lrit::SegmentIdentificationHeader>();
    return s.segmentNumber < ts.segmentNumber;
  });
  files_.insert(pos, f);
}

std::map<unsigned int, float> GOESRProduct::loadImageDataFunction() const {
  std::map<unsigned int, float> out;

  if (!hasHeader<lrit::ImageDataFunctionHeader>()) {
    return out;
  }

  auto h = getHeader<lrit::ImageDataFunctionHeader>();

  // Sample IDF (ABI Channel 8), output with lritdump -v
  //
  // Image data function (3):
  //  Data:
  //    $HALFTONE:=8
  //    _NAME:=toa_brightness_temperature
  //    _UNIT:=K
  //    255:=138.0500
  //    254:=138.7260
  //    253:=139.4020
  //    252:=140.0780
  //    251:=140.7540
  // [...]
  //    5:=307.0494
  //    4:=307.7254
  //    3:=308.4014
  //    2:=309.0774
  //    1:=309.7534
  //    0:=310.4294

  const auto str = std::string((const char*) h.data.data(), h.data.size());
  std::istringstream iss(str);
  std::string line;

  long int ki;
  float vf;

  while (std::getline(iss, line, '\n')) {
    std::istringstream lss(line);
    std::string k, v;
    std::getline(lss, k, '=');
    std::getline(lss, v, '\n');
    k.erase(k.end() - 1);

    // Exceptions thrown for any non-numeric key/value pair, but
    // we can just discard them.
    try {
      ki = std::stoi(k);
      vf = std::stof(v);
      out[ki] = vf;
    } catch(std::invalid_argument &e) {
    }
  }

  return out;
}

FilenameBuilder GOESRProduct::getFilenameBuilder(const Config::Handler& config) const {
  FilenameBuilder fb;
  fb.dir = config.dir;
  fb.filename = removeSuffix(getHeader<lrit::AnnotationHeader>().text);
  fb.time = frameStart_;
  fb.product = product_;
  fb.region = region_;
  fb.channel = channel_;
  return fb;
}

uint16_t GOESRProduct::imageIdentifier() const {
  if (!hasHeader<lrit::SegmentIdentificationHeader>()) {
    throw std::runtime_error("LRIT file has no SegmentIdentificationHeader");
  }
  auto sih = getHeader<lrit::SegmentIdentificationHeader>();
  return sih.imageIdentifier;
}

bool GOESRProduct::isSegmented() const {
  return segmented_;
}

bool GOESRProduct::isComplete() const {
  if (!isSegmented()) {
    return true;
  }

  auto sih = getHeader<lrit::SegmentIdentificationHeader>();
  return files_.size() == sih.maxSegment;
}

std::unique_ptr<Image> GOESRProduct::getImage(const Config::Handler& config) const {
  std::unique_ptr<Image> image;
  if (files_.size() == 1) {
    image = Image::createFromFile(files_[0]);
  } else {
    image = Image::createFromFiles(files_);
  }

  // This turns the white fills outside the disk black
  image->fillSides();

  // Remap image values if configured for this channel
  auto it = config.remap.find(channel_.nameShort);
  if (it != std::end(config.remap)) {
    image->remap(it->second);
  }

  return image;
}

bool GOESRProduct::matchSatelliteID(int satelliteID) const {
  return satelliteID == satelliteID_;
}

bool GOESRProduct::matchProduct(const std::vector<std::string>& products) const {
  if (products.empty()) {
    return true;
  }

  // Check for a positive match.
  const auto begin = std::begin(products);
  const auto end = std::end(products);
  const auto it = std::find(begin, end, product_.nameShort);
  if (it != end) {
    return true;
  }

  // Check for a negative match.
  bool performed_negative_check = false;
  for (const auto& product : products) {
    if (product.empty() || product.at(0) != '^') {
      continue;
    }
    // Compare the sub-string after the negation character.
    performed_negative_check = true;
    if (product.substr(1) == product_.nameShort) {
      return false;
    }
  }

  return performed_negative_check;
}

bool GOESRProduct::matchRegion(const std::vector<std::string>& regions) const {
  if (regions.empty()) {
    return true;
  }

  const auto begin = std::begin(regions);
  const auto end = std::end(regions);
  const auto it = std::find(begin, end, region_.nameShort);
  return it != end;
}

bool GOESRProduct::matchChannel(
    const std::vector<std::string>& channels) const {
  if (channels.empty()) {
    return true;
  }

  const auto begin = std::begin(channels);
  const auto end = std::end(channels);
  const auto it = std::find(begin, end, channel_.nameShort);
  return it != end;
}

SegmentKey GOESRProduct::generateKey() const {
  return std::make_tuple(product_.nameShort, region_.nameShort, channel_.nameShort);
}

GOESRImageHandler::GOESRImageHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
  for (auto& product : config_.products) {
    product = toUpper(product);
  }
  for (auto& region : config_.regions) {
    region = toUpper(region);
  }
  for (auto& channel : config_.channels) {
    channel = toUpper(channel);
  }

  if (config_.origin == "goes16") {
    satelliteID_ = 16;
  } else if (config_.origin == "goes17") {
    satelliteID_ = 17;
  } else {
    ASSERT(false);
  }
}

void GOESRImageHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 0) {
    return;
  }

  // Filter by product
  //
  // We assume that the NOAA product ID can be either 16 or 17,
  // indicating GOES-16 and GOES-17. The latter has not yet been
  // spotted in the wild as of July 2018. Even GOES-17 products
  // still used product ID equal to 16 at this time.
  //
  // Actual filtering based on satellite is done based on the details
  // encoded in the ancillary data field.
  //
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID != 16 && nlh.productID != 17) {
    return;
  }

  // Require presence of ancillary text header.
  //
  // The details expect this header to be present. It appears not to
  // be present on products other than cloud and moisture image
  // products (included in the HRIT stream starting late May 2018).
  //
  // The first GOES-16 image file that did not have this header was
  // named: OR_ABI-L2-RRQPEF-M3_G16_[...]_rescaled.lrit and I saw it
  // on the stream on 2018-05-21T18:50:00Z.
  //
  if (!f->hasHeader<lrit::AncillaryTextHeader>()) {
    return;
  }

  auto tmp = GOESRProduct(f);

  // Filter by product details
  if (!tmp.matchSatelliteID(satelliteID_) ||
      !tmp.matchProduct(config_.products) ||
      !tmp.matchRegion(config_.regions) ||
      !tmp.matchChannel(config_.channels)) {
    return;
  }

  // If this is not a segmented image we can post process immediately
  if (!tmp.isSegmented()) {
    handleImage(std::move(tmp));
    return;
  }

  // For consistency, assume that every file where the details
  // indicate it is part of a segmented image has a segment
  // identification header.
  if (!f->hasHeader<lrit::SegmentIdentificationHeader>()) {
    return;
  }

  const auto key = tmp.generateKey();

  // Find existing product with this region and channel
  auto it = products_.find(key);
  if (it == products_.end()) {
    // No existing product found; use this one as the first one
    products_[key] = std::move(tmp);
  } else {
    // If the current segment has the same image identifier as the
    // segments we have seen before, add it. Otherwise, we can
    // safely assume the existing image is incomplete and we drop it.
    auto& product = it->second;
    if (product.imageIdentifier() == tmp.imageIdentifier()) {
      product.add(f);
    } else {
      // TODO: Log that we drop the existing image
      products_[key] = std::move(tmp);
    }
  }

  // If the product is complete we can post process it
  auto& product = products_[key];
  if (product.isComplete()) {
    handleImage(std::move(product));
    products_.erase(key);
    return;
  }
}

void GOESRImageHandler::handleImage(GOESRProduct product) {
  Timer t;

  // If this handler is configured to produce false color images
  // we pass the image to a separate handler.
  if (config_.lut.data) {
    handleImageForFalseColor(std::move(product));
    return;
  }

  auto image = product.getImage(config_);
  auto fb = product.getFilenameBuilder(config_);

  // If there's a parametric gradient configured, use it in
  // combination with the LRIT ImageDataFunction to map
  // CMIP grey levels to temperature units (Kelvin), then map
  // those temperatures onto the RGB gradient.
  const auto& productName = product.getProduct().nameShort;
  auto grad = config_.gradient.find(productName);

  // For CMIP files we use the channel name to find the gradient.
  if (grad == std::end(config_.gradient) && productName == "CMIP") {
    grad = config_.gradient.find(product.getChannel().nameShort);
  }

  auto idf = product.loadImageDataFunction();

  // This is stored in an 256x1 RGB matrix for use in Image::remap()
  if (grad != std::end(config_.gradient) && idf.size() == 256) {
    cv::Mat gradientMap(256, 1, CV_8UC3);
    for (const auto& i : idf) {
      auto p = grad->second.interpolate(i.second, config_.lerptype);
      gradientMap.data[i.first * 3] = p.rgb[2] * 255;
      gradientMap.data[i.first * 3 + 1] = p.rgb[1] * 255;
      gradientMap.data[i.first * 3 + 2] = p.rgb[0] * 255;
    }
    image->remap(gradientMap);
  }

  auto mat = image->getRawImage();
  overlayMaps(product, mat);
  auto path = fb.build(config_.filename, config_.format);
  fileWriter_->write(path, mat, &t);
  if (config_.json) {
    fileWriter_->writeHeader(product.firstFile(), path);
  }
}

void GOESRImageHandler::handleImageForFalseColor(GOESRProduct p1) {
  Timer t;

  const auto key = p1.getRegion().nameShort;
  if (falseColor_.find(key) == falseColor_.end()) {
    falseColor_[key] = std::move(p1);
    return;
  }

  // Move existing product into local scope such that the local
  // one is the only remaining reference to this product.
  auto p0 = std::move(falseColor_[key]);
  falseColor_.erase(key);

  // Verify that observation time is identical.
  if (p0.getFrameStart().tv_sec != p1.getFrameStart().tv_sec) {
    falseColor_[key] = std::move(p1);
    return;
  }

  // If the channels are the same, there has been duplication on the
  // packet stream and we can ignore the latest one.
  if (p0.getChannel().nameShort == p1.getChannel().nameShort) {
    falseColor_[key] = std::move(p0);
    return;
  }

  // Swap if ordering of products doesn't match ordering of channels
  if (p0.getChannel().nameShort != config_.channels.front()) {
    std::swap(p0, p1);
  }

  // Use filename builder of first channel
  auto fb = p0.getFilenameBuilder(config_);

  // Update filename in filename builder to reflect that this is a
  // synthesized image. It would be misleading to use the filename of
  // either one of the input files.
  //
  // For example: in OR_ABI-L2-CMIPF-M3C13_G16_[...] the C13 is
  // replaced by CFC.
  //
  auto parts = split(fb.filename, '_');
  auto pos = parts[1].rfind('C');
  ASSERT(pos != std::string::npos);
  parts[1] = parts[1].substr(0, pos) + "CFC";
  fb.filename = join(parts, '_');

  // Replace channel field in filename builder.
  // The incoming filename builder will have the channel set to one of
  // the two channels used for this false color image.
  fb.channel.nameShort = "FC";
  fb.channel.nameLong = "False Color";

  // Generate false color image.
  auto i0 = p0.getImage(config_);
  auto i1 = p1.getImage(config_);
  auto out = Image::generateFalseColor(i0, i1, config_.lut);
  i0.reset();
  i1.reset();

  auto mat = out->getRawImage();
  overlayMaps(p0, mat);
  auto path = fb.build(config_.filename, config_.format);
  fileWriter_->write(path, mat, &t);
  if (config_.json) {
    fileWriter_->writeHeader(p0.firstFile(), path);
  }
}

void GOESRImageHandler::overlayMaps(const GOESRProduct& product, cv::Mat& mat) {
#ifdef HAS_PROJ
  if (config_.maps.empty()) {
    return;
  }

  auto inh = product.getHeader<lrit::ImageNavigationHeader>();
  auto lon = inh.getLongitude();

  // GOES-16 reports -75.2 but is actually at -75.0
  if (fabsf(lon - (-75.2)) < 1e-3) {
    lon = -75.0;
  }

  // GOES-17 reports -137.2 but is actually at -137.0
  if (fabsf(lon - (-137.2)) < 1e-3) {
    lon = -137.0;
  }

  // TODO: The map drawer should be cached by construction parameters.
  auto drawer = MapDrawer(&config_, lon, inh);
  mat = drawer.draw(mat);
#endif
}
