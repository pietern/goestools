#include "image.h"

#include <algorithm>
#include <cmath>

Image::Image(uint16_t imageIdentifier, std::vector<File> files)
  : imageIdentifier_(imageIdentifier),
    files_(files) {
  // Sort files by their segment number
  std::sort(
    files_.begin(),
    files_.end(),
    [](const auto& a, const auto& b) -> bool {
      auto sa = a.template getHeader<LRIT::SegmentIdentificationHeader>();
      auto sb = b.template getHeader<LRIT::SegmentIdentificationHeader>();
      return sa.segmentNumber < sb.segmentNumber;
    });

  maxSegment_ = 0;
  columns_ = 0;
  lines_ = 0;
  for (unsigned i = 0; i < files_.size(); i++) {
    const auto& f = files_[i];
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

std::string Image::getSatellite() const {
  std::string satellite;
  if (nlh_.productID == 13) {
    satellite = "GOES13";
  } else if (nlh_.productID == 15) {
    satellite = "GOES15";
  } else {
    assert(false);
  }
  return satellite;
}

std::string Image::getProductShort() const {
  std::string product;
  if (nlh_.productSubID % 10 == 1) {
    product = "FD";
  } else if (nlh_.productSubID % 10 == 2) {
    product = "NH";
  } else if (nlh_.productSubID % 10 == 3) {
    product = "SH";
  } else if (nlh_.productSubID % 10 == 4) {
    product = "US";
  } else {
    std::array<char, 8> buf;
    auto num = (nlh_.productSubID % 10) - 5;
    auto len = snprintf(buf.data(), buf.size(), "SI%02d_", num);
    product = std::string(buf.data(), len);
  }
  return product;
}

std::string Image::getProductLong() const {
  std::string product;
  if (nlh_.productSubID % 10 == 1) {
    product = "Full Disk";
  } else if (nlh_.productSubID % 10 == 2) {
    product = "Northern Hemisphere";
  } else if (nlh_.productSubID % 10 == 3) {
    product = "Southern Hemisphere";
  } else if (nlh_.productSubID % 10 == 4) {
    product = "United States";
  } else {
    product = "Special Interest ";
    std::array<char, 8> buf;
    auto num = (nlh_.productSubID % 10) - 5;
    auto len = snprintf(buf.data(), buf.size(), "%d", num);
    product += std::string(buf.data(), len);
  }
  return product;
}

std::string Image::getChannelShort() const {
  std::string channel;
  if (nlh_.productSubID <= 10) {
    channel = "IR";
  } else if (nlh_.productSubID <= 20) {
    channel = "VS";
  } else {
    channel = "WV";
  }
  return channel;
}

std::string Image::getChannelLong() const {
  std::string channel;
  if (nlh_.productSubID <= 10) {
    channel = "Infrared";
  } else if (nlh_.productSubID <= 20) {
    channel = "Visible";
  } else {
    channel = "Water Vapor";
  }
  return channel;
}

std::string Image::getTimeShort() const {
  std::array<char, 128> tsbuf;
  auto ts = files_[0].getHeader<LRIT::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y%m%d-%H%M%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::string Image::getTimeLong() const {
  std::array<char, 128> tsbuf;
  auto ts = files_[0].getHeader<LRIT::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y-%m-%d %H:%M:%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::string Image::getBasename() const {
  return
    getSatellite() + "_" +
    getProductShort() + "_" +
    getChannelShort() + "_" +
    getTimeShort();
}

cv::Mat Image::getRawImage() const {
  cv::Mat raw(lines_, columns_, CV_8UC1);
  size_t offset = 0;
  for (const auto& f : files_) {
    auto is = f.getHeader<LRIT::ImageStructureHeader>();
    auto ifs = f.getData();
    for (auto line = 0; line < is.lines; line++) {
      ifs.read((char*)raw.data + offset, is.columns);
      assert(ifs);
      offset += is.columns;

      // Overwrite the first line with the second line.
      // The first line contains garbage, but we have to put
      // something in its place.
      if (line == 1) {
        memcpy(
          raw.data + offset - (2 * is.columns),
          raw.data + offset - is.columns,
          is.columns);
      }
    }
  }
  return raw;
}

cv::Mat Image::getRawImage(const Area& roi) const {
  cv::Mat raw = getRawImage();
  int x = roi.minColumn - area_.minColumn;
  int y = roi.minLine - area_.minLine;
  int w = roi.maxColumn - roi.minColumn;
  int h = roi.maxLine - roi.minLine;
  cv::Rect crop(x, y, w, h);
  return raw(crop);
}

// Scale columns/lines per scaling factor in ImageNavigationHeader
cv::Size Image::scaleSize(cv::Size s, bool shrink) const {
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

cv::Mat Image::getScaledImage(bool shrink) const {
  cv::Mat raw = getRawImage();
  cv::Mat out(scaleSize(cv::Size(raw.cols, raw.rows), shrink), CV_8UC1);
  cv::resize(raw, out, out.size());
  return std::move(out);
}

cv::Mat Image::getScaledImage(const Area& roi, bool shrink) const {
  cv::Mat raw = getRawImage(roi);
  cv::Mat out(scaleSize(cv::Size(raw.cols, raw.rows), shrink), CV_8UC1);
  cv::resize(raw, out, out.size());
  return std::move(out);
}
