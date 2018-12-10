#pragma once

#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <vector>

#include <util/error.h>

namespace lrit {

using HeaderMap = std::map<int, int>;
using Buffer = std::vector<uint8_t>;

struct PrimaryHeader {
  static constexpr int CODE = 0;

  uint8_t headerType;
  uint16_t headerLength;
  uint8_t fileType;
  uint32_t totalHeaderLength;
  uint64_t dataLength;
};

struct ImageStructureHeader {
  static constexpr int CODE = 1;

  uint8_t headerType;
  uint16_t headerLength;
  uint8_t bitsPerPixel;
  uint16_t columns;
  uint16_t lines;
  uint8_t compression;
};

struct ImageNavigationHeader {
  static constexpr int CODE = 2;

  uint8_t headerType;
  uint16_t headerLength;
  char projectionName[32];
  uint32_t columnScaling;
  uint32_t lineScaling;
  uint32_t columnOffset;
  uint32_t lineOffset;

  // Converts projection name into floating point longitude.
  float getLongitude() const;
};

struct ImageDataFunctionHeader {
  static constexpr int CODE = 3;

  uint8_t headerType;
  uint16_t headerLength;
  std::vector<uint8_t> data;
};

struct AnnotationHeader {
  static constexpr int CODE = 4;

  uint8_t headerType;
  uint16_t headerLength;
  std::string text;
};

struct TimeStampHeader {
  static constexpr int CODE = 5;

  uint8_t headerType;
  uint16_t headerLength;
  char ccsds[7];

  // Converts CCSDS time into UNIX timespec.
  struct timespec getUnix() const;

  std::string getTimeShort() const;
  std::string getTimeLong() const;
};

struct AncillaryTextHeader {
  static constexpr int CODE = 6;

  uint8_t headerType;
  uint16_t headerLength;
  std::string text;
};

struct KeyHeader {
  static constexpr int CODE = 7;

  uint8_t headerType;
  uint16_t headerLength;
};

// Mission specific header.
// See: http://www.noaasis.noaa.gov/LRIT/pdf-files/LRIT_receiver-specs.pdf
struct SegmentIdentificationHeader {
  static constexpr int CODE = 128;

  uint8_t headerType;
  uint16_t headerLength;
  uint16_t imageIdentifier;
  uint16_t segmentNumber;
  uint16_t segmentStartColumn;
  uint16_t segmentStartLine;
  uint16_t maxSegment;
  uint16_t maxColumn;
  uint16_t maxLine;
};

struct NOAALRITHeader {
  static constexpr int CODE = 129;

  uint8_t headerType;
  uint16_t headerLength;
  char agencySignature[4];
  uint16_t productID;
  uint16_t productSubID;
  uint16_t parameter;
  uint8_t noaaSpecificCompression;
};

struct HeaderStructureRecordHeader {
  static constexpr int CODE = 130;

  uint8_t headerType;
  uint16_t headerLength;
  std::string headerStructure;
};

struct RiceCompressionHeader {
  static constexpr int CODE = 131;

  uint8_t headerType;
  uint16_t headerLength;
  uint16_t flags;
  uint8_t pixelsPerBlock;
  uint8_t scanLinesPerPacket;
};

struct DCSFileNameHeader {
  static constexpr int CODE = 132;

  uint8_t headerType;
  uint16_t headerLength;
  std::string fileName;
};

std::map<int, int> getHeaderMap(const Buffer& b);

template <typename H>
inline bool hasHeader(const HeaderMap& m) {
  auto it = m.find(H::CODE);
  return it != m.end();
}

template <typename H>
H getHeader(const Buffer& b, int pos);

template <typename H>
H getHeader(const Buffer& b, const HeaderMap& m) {
  auto it = m.find(H::CODE);
  ASSERT(it != m.end());
  return getHeader<H>(b, it->second);
}

} // namespace lrit
