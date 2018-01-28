#include "image.h"

#include <cassert>

namespace {

cv::Mat getRawImage(const lrit::File& f) {
  cv::Mat raw;

  auto ph = f.getHeader<lrit::PrimaryHeader>();
  auto ish = f.getHeader<lrit::ImageStructureHeader>();
  auto ifs = f.getData();
  raw = cv::Mat(ish.lines, ish.columns, CV_8UC1);

  if (ish.bitsPerPixel == 1) {
    // Number of pixels
    unsigned long n = (raw.size().width * raw.size().height);
    assert(ph.dataLength == n);

    // Read everything in one shot
    // Round up to nearest multiple of 8 because we're reading bytes
    auto line = std::make_unique<char[]>((n + 7) / 8);
    ifs->read(line.get(), (n + 7) / 8);
    assert(*ifs);

    // Pixel by pixel
    for (unsigned long i = 0; i < n; i += 8) {
      auto byte = line.get()[i / 8];
      for (auto j = i; j < (i + 8); j++) {
        if (byte & 0x80) {
          ((char*)raw.data)[j] = (char)0xff;
        } else {
          ((char*)raw.data)[j] = (char)0x00;
        }
        byte <<= 1;
      }
    }
  } else if (ish.bitsPerPixel == 8) {
    ifs->read((char*)raw.data, raw.size().width * raw.size().height);
    assert(ifs);
  } else {
    std::cerr << "bitsPerPixel == " << ish.bitsPerPixel << std::endl;
    assert(false);
  }

  return raw;
}

} // namespace

std::unique_ptr<Image> Image::createFromFile(
    std::shared_ptr<const lrit::File> f) {
  auto raw = getRawImage(*f);
  return std::make_unique<Image>(*f, raw);
}

std::unique_ptr<Image> Image::createFromFiles(
    std::vector<std::shared_ptr<const lrit::File> > fs) {
  uint16_t columns = 0;
  uint16_t lines = 0;

  // Assert these are identical across all segments
  for (const auto& f : fs) {
    auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();
    if (columns == 0) {
      columns = sih.maxColumn;
    } else {
      assert(sih.maxColumn == columns);
    }
    if (lines == 0) {
      lines = sih.maxLine;
    } else {
      assert(sih.maxLine == lines);
    }
  }

  cv::Mat raw(lines, columns, CV_8UC1);
  for (const auto& f : fs) {
    auto ish = f->getHeader<lrit::ImageStructureHeader>();
    auto sih = f->getHeader<lrit::SegmentIdentificationHeader>();

    char* chunk = (char*) raw.data + (sih.segmentStartLine * columns);
    auto ifs = f->getData();
    ifs->read(chunk, ish.lines * ish.columns);
    assert(*ifs);

    // Fill the contour with black pixels (left side)
    for (auto y = 0; y < ish.lines; y++) {
      uint8_t* data = (uint8_t*) chunk + y * ish.columns;
      for (auto x = 0; x < ish.columns / 2; x++) {
        if (data[x] == 0xff) {
          data[x] = 0x00;
        } else {
          break;
        }
      }
      for (auto x = ish.columns - 1; x >= ish.columns / 2; x--) {
        if (data[x] == 0xff) {
          data[x] = 0x00;
        } else {
          break;
        }
      }
    }
  }

  return std::make_unique<Image>(*fs[0], raw);
}

Image::Image(const lrit::File& f, cv::Mat m) : m_(m) {
}

void Image::save(const std::string& path) const {
  cv::imwrite(path, m_);
}
