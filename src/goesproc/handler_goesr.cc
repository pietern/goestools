#include "handler_goesr.h"

#include <cassert>

#include "lib/util.h"

#include "string.h"

GOESRImageHandler::GOESRImageHandler(
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

  if (config_.product == "goes16") {
    satelliteID_ = 16;
  } else if (config_.product == "goes17") {
    satelliteID_ = 17;
  } else {
    assert(false);
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

  auto details = loadDetails(*f);

  // Filter by satellite
  if (satelliteID_ != details.satelliteID) {
    return;
  }

  // Filter by region
  if (!config_.regions.empty()) {
    auto begin = std::begin(config_.regions);
    auto end = std::end(config_.regions);
    auto it = std::find(begin, end, details.region.nameShort);
    if (it == end) {
      return;
    }
  }

  // Filter by channel
  if (!config_.channels.empty()) {
    auto begin = std::begin(config_.channels);
    auto end = std::end(config_.channels);
    auto it = std::find(begin, end, details.channel.nameShort);
    if (it == end) {
      return;
    }
  }

  // Prepare filename builder. It is done here so we don't need to
  // pass the shared_ptr to lrit::File around.
  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.filename = removeSuffix(f->getHeader<lrit::AnnotationHeader>().text);
  fb.time = details.frameStart;
  fb.region = details.region;
  fb.channel = details.channel;

  // Process image data function
  if (f->hasHeader<lrit::ImageDataFunctionHeader>()) {
    auto h = f->getHeader<lrit::ImageDataFunctionHeader>();

    const auto str = std::string((const char*) h.data.data(), h.data.size());
    std::istringstream iss(str);
    std::string line;

    unsigned int ki;
    float vf;

    while (std::getline(iss, line, '\n')) {
      std::istringstream lss(line);
      std::string k, v;
      std::getline(lss, k, '=');
      std::getline(lss, v, '=');
      k.erase(k.end() - 1);
      ki = atoi(k.c_str());
      vf = atof(v.c_str());
      imageDataFunction_[ki] = vf;
    }
  }

  // If this is not a segmented image we can post process immediately
  if (!details.segmented) {
    auto image = Image::createFromFile(f);
    auto tuple = Tuple(std::move(image), std::move(details), std::move(fb));
    handleImage(std::move(tuple));
    return;
  }

  // Assume all images are segmented
  if (!f->hasHeader<lrit::SegmentIdentificationHeader>()) {
    return;
  }

  auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();
  const auto key = std::make_pair(
    details.region.nameShort,
    details.channel.nameShort);
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
    auto image = Image::createFromFiles(vector);
    vector.clear();
    auto tuple = Tuple(std::move(image), std::move(details), std::move(fb));
    handleImage(std::move(tuple));
    return;
  }
}

void GOESRImageHandler::handleImage(Tuple t) {
  auto& image = t.image;
  auto& details = t.details;
  auto& fb = t.fb;

  // This turns the white fills outside the disk black
  image->fillSides();

  // Remap image values if configured for this channel
  auto it = config_.remap.find(details.channel.nameShort);
  if (it != std::end(config_.remap)) {
    image->remap(it->second);
  }

  auto grad = config_.gradient.find(details.channel.nameShort);
  auto idf = imageDataFunction_.begin();

  // If there's a parametric gradient set, use it to generate
  // an RGB gradient map LUT, then apply it to the image
  if (grad != std::end(config_.gradient) && idf != imageDataFunction_.end()) {
    cv::Mat gradientMap(256, 1, CV_8UC3);
    for (auto i = idf; i != imageDataFunction_.end(); i++) {
      auto p = grad->second.interpolate(i->second, config_.lerptype);
      gradientMap.data[i->first * 3] = p.rgb[2] * 255;
      gradientMap.data[i->first * 3 + 1] = p.rgb[1] * 255;
      gradientMap.data[i->first * 3 + 2] = p.rgb[0] * 255;
    }
    image->remap(gradientMap);
  }

  // If this handler is configured to produce false color images,
  // we may need to wait for the other channel to come along.
  if (config_.lut.data) {
    handleImageForFalseColor(std::move(t));
    return;
  }

  auto path = fb.build(config_.filename, config_.format);
  fileWriter_->write(path, image->getRawImage());
}

void GOESRImageHandler::handleImageForFalseColor(Tuple t1) {
  const auto key = t1.details.region.nameShort;
  auto& t0 = falseColor_[key];

  // References to existing tuple
  auto& i0 = t0.image;
  auto& d0 = t0.details;
  auto& f0 = t0.fb;

  // References to new tuple
  auto& i1 = t1.image;
  auto& d1 = t1.details;
  auto& f1 = t1.fb;

  // If this is the first image of a pair, keep it around and wait for
  // the next one.
  if (!i0 || (d0.frameStart.tv_sec != d1.frameStart.tv_sec)) {
    falseColor_[key] = std::move(t1);
    return;
  }

  // If the channels are the same, there has been duplication on the
  // packet stream and we can ignore the latest one.
  if (d0.channel.nameShort == d1.channel.nameShort) {
    return;
  }

  // Swap if ordering of images doesn't match ordering of channels
  if (d0.channel.nameShort != config_.channels.front()) {
    std::swap(i0, i1);
    std::swap(d0, d1);
    std::swap(f0, f1);
  }

  // Use filename builder of first channel
  auto& fb = f0;

  // Update filename in filename builder to reflect that this is a
  // synthesized image. It would be misleading to use the filename of
  // either one of the input files.
  //
  // For example: in OR_ABI-L2-CMIPF-M3C13_G16_[...] the C13 is
  // replaced by CFC.
  //
  auto parts = split(fb.filename, '_');
  auto pos = parts[1].rfind('C');
  assert(pos != std::string::npos);
  parts[1] = parts[1].substr(0, pos) + "CFC";
  fb.filename = join(parts, '_');

  // Replace channel field in filename builder.
  // The incoming filename builder will have the channel set to one of
  // the two channels used for this false color image.
  fb.channel.nameShort = "FC";
  fb.channel.nameLong = "False Color";

  auto image = Image::generateFalseColor(i0, i1, config_.lut);
  auto path = fb.build(config_.filename, config_.format);
  fileWriter_->write(path, image->getRawImage());

  // Remove existing tuple from temporary storage
  falseColor_.erase(key);
}

GOESRImageHandler::Details GOESRImageHandler::loadDetails(const lrit::File& f) {
  Details details;

  auto text = f.getHeader<lrit::AncillaryTextHeader>().text;
  auto pairs = split(text, ';');
  for (const auto& pair : pairs) {
    auto elements = split(pair, '=');
    assert(elements.size() == 2);
    auto key = trimRight(elements[0]);
    auto value = trimLeft(elements[1]);

    if (key == "Time of frame start") {
      auto ok = parseTime(value, &details.frameStart);
      assert(ok);
      continue;
    }

    if (key == "Satellite") {
      details.satellite = value;

      // First skip over non-digits.
      // Expect a value of "G16" or "G17".
      std::stringstream ss(value);
      while (!isdigit(ss.peek())) {
        ss.get();
      }
      ss >> details.satelliteID;
      continue;
    }

    if (key == "Instrument") {
      details.instrument = value;
      continue;
    }

    if (key == "Channel") {
      std::array<char, 32> buf;
      size_t len;
      auto num = std::stoi(value);
      assert(num >= 1 && num <= 16);
      len = snprintf(buf.data(), buf.size(), "CH%02d", num);
      details.channel.nameShort = std::string(buf.data(), len);
      len = snprintf(buf.data(), buf.size(), "Channel %d", num);
      details.channel.nameLong = std::string(buf.data(), len);
      continue;
    }

    if (key == "Imaging Mode") {
      details.imagingMode = value;
      continue;
    }

    if (key == "Region") {
      // Ignore; this needs to be derived from the file name
      // because the mesoscale sector is not included here.
      continue;
    }

    if (key == "Resolution") {
      details.resolution = value;
      continue;
    }

    if (key == "Segmented") {
      details.segmented = (value == "yes");
      continue;
    }

    std::cerr << "Unhandled key in ancillary text: " << key << std::endl;
    assert(false);
  }

  // Tell apart the two mesoscale sectors
  {
    auto fileName = f.getHeader<lrit::AnnotationHeader>().text;
    auto parts = split(fileName, '-');
    assert(parts.size() >= 4);
    if (parts[2] == "CMIPF") {
      details.region.nameLong = "Full Disk";
      details.region.nameShort = "FD";
    } else if (parts[2] == "CMIPM1") {
      details.region.nameLong = "Mesoscale 1";
      details.region.nameShort = "M1";
    } else if (parts[2] == "CMIPM2") {
      details.region.nameLong = "Mesoscale 2";
      details.region.nameShort = "M2";
    } else {
      std::cerr << "Unable to derive region from: " << parts[2] << std::endl;
      assert(false);
    }
  }

  return details;
}
