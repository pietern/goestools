#include "handler_goes16.h"

#include <cassert>

#include "lib/util.h"

#include "string.h"

GOES16ImageHandler::GOES16ImageHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
  config_.region = toUpper(config_.region);
  for (auto& channel : config_.channels) {
    channel = toUpper(channel);
  }
}

void GOES16ImageHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 0) {
    return;
  }

  // Filter GOES-16
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID != 16) {
    return;
  }

  auto details = loadDetails(*f);

  // Filter by region
  if (config_.region != details.region.nameShort) {
    return;
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
  fb.region = &details.region;
  fb.channel = &details.channel;

  // If this is not a segmented image we can post process immediately
  if (!details.segmented) {
    auto image = Image::createFromFile(f);
    handleImage(fb, std::move(image), details);
    return;
  }

  assert(f->hasHeader<lrit::SegmentIdentificationHeader>());
  auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();
  auto& map = segments_[details.channel.nameShort];
  auto& vector = map[sih.imageIdentifier];
  vector.push_back(f);
  if (vector.size() == sih.maxSegment) {
    auto image = Image::createFromFiles(vector);

    // Fill sides with black only for GOES-16
    image->fillSides();
    handleImage(fb, std::move(image), details);

    // Remove from handler cache
    map.erase(sih.imageIdentifier);
    return;
  }
}

void GOES16ImageHandler::handleImage(
    const FilenameBuilder& fb,
    std::unique_ptr<Image> image,
    GOES16ImageHandler::Details details) {
  // Remap image values if configured for this channel
  auto it = config_.remap.find(details.channel.nameShort);
  if (it != std::end(config_.remap)) {
    image->remap(it->second);
  }

  // If this handler is configured to produce false color images,
  // we may need to wait for the other channel to come along.
  if (config_.lut.data) {
    handleImageForFalseColor(fb, std::move(image), details);
    return;
  }

  auto path = fb.build(config_.filename, config_.format);
  fileWriter_->write(path, image->getRawImage());
}

void GOES16ImageHandler::handleImageForFalseColor(
    const FilenameBuilder& fb,
    std::unique_ptr<Image> i1,
    GOES16ImageHandler::Details d1) {
  auto i0 = std::move(std::get<0>(tmp_));
  auto d0 = std::move(std::get<1>(tmp_));

  // If this is the first image of a pair, keep it around and wait for the next one.
  if (!i0 || (d0.frameStart.tv_sec != d1.frameStart.tv_sec)) {
    tmp_ = std::make_tuple<std::unique_ptr<Image>, Details>(std::move(i1), std::move(d1));
    return;
  }

  // Swap if ordering of images doesn't match ordering of channels
  if (d0.channel.nameShort != config_.channels.front()) {
    i0.swap(i1);
  }

  auto image = Image::generateFalseColor(std::move(i0), std::move(i1), config_.lut);
  auto path = fb.build(config_.filename, config_.format);
  fileWriter_->write(path, image->getRawImage());
}

GOES16ImageHandler::Details GOES16ImageHandler::loadDetails(const lrit::File& f) {
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
