#include "lib/lrit.h"

#include <cassert>
#include <cstring>

namespace LRIT {

const int PrimaryHeader::CODE = 0;
const int ImageStructureHeader::CODE = 1;
const int ImageNavigationHeader::CODE = 2;
const int ImageDataFunctionHeader::CODE = 3;
const int AnnotationHeader::CODE = 4;
const int TimeStampHeader::CODE = 5;
const int AncillaryTextHeader::CODE = 6;
const int KeyHeader::CODE = 7;
const int SegmentIdentificationHeader::CODE = 128;
const int NOAALRITHeader::CODE = 129;
const int HeaderStructureRecordHeader::CODE = 130;
const int RiceCompressionHeader::CODE = 131;

// The days field in CCSDS time starts counting on January 1, 1958.
// Unix time starts counting on January 1, 1970.
constexpr auto ccsdsToUnixDaysOffset = 4383;

struct timespec TimeStampHeader::getUnix() {
  struct timespec ts{0, 0};
  uint16_t days = __builtin_bswap16(*reinterpret_cast<const uint16_t*>(&ccsds[1]));
  uint32_t millis = __builtin_bswap32(*reinterpret_cast<const uint32_t*>(&ccsds[3]));
  if (days > 0 || millis > 0) {
    assert(days >= ccsdsToUnixDaysOffset);
    ts.tv_sec = ((days - ccsdsToUnixDaysOffset) * 24 * 60 * 60) + (millis / 1000);
    ts.tv_nsec = (millis % 1000) * 1000 * 1000;
  }
  return ts;
}

std::map<int, int> getHeaderMap(const Buffer& b) {
  uint8_t headerType;
  uint16_t headerLength;
  uint32_t totalHeaderLength;
  uint32_t pos = 0;

  headerType = b[0];
  assert(headerType == 0);
  headerLength = (b[1] << 8) | b[2];
  assert(headerLength == 16);
  memcpy(&totalHeaderLength, &b[4], 4);
  totalHeaderLength = __builtin_bswap32(totalHeaderLength);

  // Find LRIT headers
  std::map<int, int> m;
  while (pos < totalHeaderLength) {
    headerType = b[pos];
    headerLength = (b[pos+1] << 8) | b[pos+2];
    m[headerType] = pos;
    pos += headerLength;
  }

  return m;
}

template <typename H>
class HeaderReader {
public:
  HeaderReader(const Buffer& b, int p) : b_(b), p_(p) {
  }

  H getHeader() {
    read(&h_.headerType);
    read(&h_.headerLength);
    return h_;
  }

  void read(char* dst, size_t len) {
    memcpy(dst, &b_[p_], len);
    p_ += len;
  }

  void read(std::vector<uint8_t>& dst, size_t len) {
    auto a = b_.begin() + p_;
    auto b = b_.begin() + p_ + len;
    dst.insert(dst.begin(), a, b);
    p_ += len;
  }

  void read(std::string& dst, size_t len) {
    auto a = b_.begin() + p_;
    auto b = b_.begin() + p_ + len;
    dst = std::string(a, b);
    p_ += len;
  }

  void read(uint8_t* dst) {
    *dst = b_[p_];
    p_ += sizeof(uint8_t);
  }

  void read(uint16_t* dst) {
    *dst = __builtin_bswap16(*reinterpret_cast<const uint16_t*>(&b_[p_]));
    p_ += sizeof(uint16_t);
  }

  void read(uint32_t* dst) {
    *dst = __builtin_bswap32(*reinterpret_cast<const uint32_t*>(&b_[p_]));
    p_ += sizeof(uint32_t);
  }

  void read(uint64_t* dst) {
    *dst = __builtin_bswap64(*reinterpret_cast<const uint64_t*>(&b_[p_]));
    p_ += sizeof(uint64_t);
  }

protected:
  const Buffer& b_;
  int p_;
  H h_;
};

template <>
PrimaryHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<PrimaryHeader>(b, pos);
  auto h = r.getHeader();
  r.read(&h.fileType);
  r.read(&h.totalHeaderLength);
  r.read(&h.dataLength);
  return h;
}

template <>
ImageStructureHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<ImageStructureHeader>(b, pos);
  auto h = r.getHeader();
  r.read(&h.bitsPerPixel);
  r.read(&h.columns);
  r.read(&h.lines);
  r.read(&h.compression);
  return h;
}

template<>
ImageNavigationHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<ImageNavigationHeader>(b, pos);
  auto h = r.getHeader();
  r.read(h.projectionName, sizeof(h.projectionName));
  r.read(&h.columnScaling);
  r.read(&h.lineScaling);
  r.read(&h.columnOffset);
  r.read(&h.lineOffset);
  return h;
}

template<>
ImageDataFunctionHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<ImageDataFunctionHeader>(b, pos);
  auto h = r.getHeader();
  r.read(h.data, h.headerLength - 3);
  return h;
}

template<>
AnnotationHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<AnnotationHeader>(b, pos);
  auto h = r.getHeader();
  r.read(h.text, h.headerLength - 3);
  return h;
}

template<>
TimeStampHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<TimeStampHeader>(b, pos);
  auto h = r.getHeader();
  r.read(h.ccsds, sizeof(h.ccsds));
  return h;
}

template<>
AncillaryTextHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<AncillaryTextHeader>(b, pos);
  auto h = r.getHeader();
  r.read(h.text, h.headerLength - 3);
  return h;
}

template<>
SegmentIdentificationHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<SegmentIdentificationHeader>(b, pos);
  auto h = r.getHeader();
  r.read(&h.imageIdentifier);
  r.read(&h.segmentNumber);
  r.read(&h.segmentStartColumn);
  r.read(&h.segmentStartLine);
  r.read(&h.maxSegment);
  r.read(&h.maxColumn);
  r.read(&h.maxLine);
  return h;
}

template<>
NOAALRITHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<NOAALRITHeader>(b, pos);
  auto h = r.getHeader();
  r.read(h.agencySignature, sizeof(h.agencySignature));
  r.read(&h.productID);
  r.read(&h.productSubID);
  r.read(&h.parameter);
  r.read(&h.noaaSpecificCompression);
  return h;
}

template<>
HeaderStructureRecordHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<HeaderStructureRecordHeader>(b, pos);
  auto h = r.getHeader();
  r.read(h.headerStructure, h.headerLength - 3);
  return h;
}

template<>
RiceCompressionHeader getHeader(const Buffer& b, int pos) {
  auto r = HeaderReader<RiceCompressionHeader>(b, pos);
  auto h = r.getHeader();
  r.read(&h.flags);
  r.read(&h.pixelsPerBlock);
  r.read(&h.scanLinesPerPacket);
  return h;
}

} // namespace LRIT
