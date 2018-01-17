#include "segmented_image.h"

#include <algorithm>
#include <cmath>

SegmentedImage::SegmentedImage(
  uint16_t imageIdentifier,
  std::vector<std::unique_ptr<Image>>&& images)
  : imageIdentifier_(imageIdentifier),
    images_(std::move(images)) {
  // Sort images by their segment number
  std::sort(
    images_.begin(),
    images_.end(),
    [](const auto& a, const auto& b) -> bool {
      auto sa = a->getFile().template getHeader<LRIT::SegmentIdentificationHeader>();
      auto sb = b->getFile().template getHeader<LRIT::SegmentIdentificationHeader>();
      return sa.segmentNumber < sb.segmentNumber;
    });

  maxSegment_ = 0;
  columns_ = 0;
  lines_ = 0;
  for (unsigned i = 0; i < images_.size(); i++) {
    const auto& f = images_[i]->getFile();
    auto is = f.getHeader<LRIT::ImageStructureHeader>();
    auto in = f.getHeader<LRIT::ImageNavigationHeader>();
    auto si = f.getHeader<LRIT::SegmentIdentificationHeader>();

    // Use first file as reference for subset of headers
    if (i == 0) {
      inh_ = in;
      nlh_ = f.getHeader<LRIT::NOAALRITHeader>();
      assert(nlh_.productSubID > 0);
      assert(nlh_.productSubID <= 30);
    }

    // Update relative area represented by this image
    Area tmp;
    tmp.minColumn = -in.columnOffset;
    tmp.maxColumn = -in.columnOffset + is.columns;
    tmp.minLine = -in.lineOffset;
    tmp.maxLine = -in.lineOffset + is.lines;

    // For Himawari-8, the offset accounting is done differently
    if (nlh_.productID == 43) {
      auto lineOffset = -in.lineOffset + si.segmentStartLine;
      tmp.minLine = lineOffset;
      tmp.maxLine = lineOffset + is.lines;
    }

    // Special case for GOES 13 -- United States: offset is off by 1.
    if (nlh_.productID == 13 && (nlh_.productSubID % 10) == 4) {
      tmp.minLine++;
      tmp.maxLine++;
    }

    if (i == 0) {
      area_ = tmp;
    } else {
      area_ = area_.getUnion(tmp);
    }

    // Know how many files to expect
    if (maxSegment_ == 0) {
      maxSegment_ = si.maxSegment;
    } else {
      // Sanity check; maxSegment must be identical across files
      assert(maxSegment_ == si.maxSegment);
    }

    // Number of columns in segments
    if (columns_ == 0) {
      columns_ = is.columns;
    } else {
      // Sanity check; only support vertical segmentation so columns
      // must be identical across files.
      assert(columns_ == is.columns);
    }

    // Number of lines in segments
    lines_ += is.lines;
  }
}

std::string SegmentedImage::getSatellite() const {
  return getImage()->getSatellite();
}

std::string SegmentedImage::getRegionShort() const {
  return getImage()->getRegionShort();
}

std::string SegmentedImage::getRegionLong() const {
  return getImage()->getRegionLong();
}

std::string SegmentedImage::getChannelShort() const {
  return getImage()->getChannelShort();
}

std::string SegmentedImage::getChannelLong() const {
  return getImage()->getChannelLong();
}

std::string SegmentedImage::getTimeShort() const {
  return getImage()->getTimeShort();
}

std::string SegmentedImage::getTimeLong() const {
  return getImage()->getTimeLong();
}

std::string SegmentedImage::getBasename() const {
  return getImage()->getBasename();
}

cv::Mat SegmentedImage::getRawImage() const {
  cv::Mat raw(lines_, columns_, CV_8UC1);
  size_t offset = 0;
  for (const auto& image : images_) {
    const auto& f = image->getFile();
    auto is = f.getHeader<LRIT::ImageStructureHeader>();
    auto ifs = f.getData();
    ifs->read((char*)raw.data + offset, is.lines * is.columns);
    assert(*ifs);
    offset += is.lines * is.columns;
  }
  return raw;
}

cv::Mat SegmentedImage::getRawImage(const Area& roi) const {
  cv::Mat raw = getRawImage();
  int x = roi.minColumn - area_.minColumn;
  int y = roi.minLine - area_.minLine;
  int w = roi.maxColumn - roi.minColumn;
  int h = roi.maxLine - roi.minLine;
  cv::Rect crop(x, y, w, h);
  return raw(crop);
}

// Scale columns/lines per scaling factor in ImageNavigationHeader
cv::Size SegmentedImage::scaleSize(cv::Size s, bool shrink) const {
  float f = ((float)inh_.columnScaling / (float)inh_.lineScaling);
  if (f < 1.0) {
    if (shrink) {
      s.height = ceilf(s.height * f);
    } else {
      s.width = ceilf(s.width / f);
    }
  } else {
    if (shrink) {
      s.width = ceilf(s.width / f);
    } else {
      s.height = ceilf(s.height * f);
    }
  }
  return s;
}

cv::Mat SegmentedImage::getScaledImage(bool shrink) const {
  cv::Mat raw = getRawImage();
  cv::Mat out(scaleSize(cv::Size(raw.cols, raw.rows), shrink), CV_8UC1);
  cv::resize(raw, out, out.size());
  return std::move(out);
}

cv::Mat SegmentedImage::getScaledImage(const Area& roi, bool shrink) const {
  cv::Mat raw = getRawImage(roi);
  cv::Mat out(scaleSize(cv::Size(raw.cols, raw.rows), shrink), CV_8UC1);
  cv::resize(raw, out, out.size());
  return std::move(out);
}
