#include "json.h"

#include <sstream>

#include <util/error.h>
#include <util/string.h>

using json = nlohmann::json;

using namespace util;

namespace lrit {

void to_json(json& j, const PrimaryHeader& h) {
  j = {
      {"FileType", int(h.fileType)},
      // LRIT value is in bytes
      {"TotalHeaderBytes", int(h.totalHeaderLength)},
      // LRIT value is in bits
      {"DataBytes", int((h.dataLength + 7) / 8)},
  };
}

void to_json(json& j, const ImageStructureHeader& h) {
  j = {
      {"BitsPerPixel", int(h.bitsPerPixel)},
      {"Columns", int(h.columns)},
      {"Lines", int(h.lines)},
      {"Compression", int(h.compression)},
  };
}

void to_json(json& j, const ImageNavigationHeader& h) {
  std::string projectionName(h.projectionName);
  auto rpos = projectionName.find(')');
  j = {
      {"ProjectionName", projectionName.substr(0, rpos + 1)},
      {"ColumnScaling", int(h.columnScaling)},
      {"LineScaling", int(h.lineScaling)},
      {"ColumnOffset", int(h.columnOffset)},
      {"LineOffset", int(h.lineOffset)},
  };
}

static bool tryDecodeImageDataFunctionHeader(json& j, const std::string& str) {
  // The image data function consists of string and numeric key value pairs.
  // The numeric pairs are a mapping from pixel values to sensor measurements.
  json map;
  std::array<float, 256> table;
  std::istringstream iss(str);
  std::string line;
  while (std::getline(iss, line, '\n')) {
    std::istringstream lss(line);
    std::string k, v;
    std::getline(lss, k, '=');
    std::getline(lss, v, '\n');
    k.erase(k.end() - 1);
    if (k.empty() || v.empty()) {
      return false;
    }
    try {
      const auto ki = std::stoi(k);
      const auto vf = std::stof(v);
      if (ki < 0 || ki >= (signed) table.size()) {
        return false;
      }
      table[ki] = vf;
    } catch(std::invalid_argument &e) {
      map[k] = v;
    }
  }
  j["Map"] = map;
  j["Table"] = table;
  return true;
}

void to_json(json& j, const ImageDataFunctionHeader& h) {
  const auto str = std::string((const char*)h.data.data(), h.data.size());
  if (!tryDecodeImageDataFunctionHeader(j, str)) {
    j["Original"] = str;
  }
}

void to_json(json& j, const AnnotationHeader& h) {
  j = h.text;
}

void to_json(json& j, const TimeStampHeader& h) {
  std::array<char, 128> tsbuf;
  const auto ts = h.getUnix();
  const auto len = strftime(
      tsbuf.data(), tsbuf.size(), "%Y-%m-%dT%H:%M:%SZ", gmtime(&ts.tv_sec));
  j = {
      {"Unix", ts.tv_sec},
      {"ISO8601", std::string(tsbuf.data(), len)},
  };
}

void to_json(json& j, const AncillaryTextHeader& h) {
  // The ancillary text header is an opaque string per the LRIT spec.
  // However, we know it has some structure and further decode it.
  auto pairs = split(h.text, ';');
  for (const auto& pair : pairs) {
    auto elements = split(pair, '=');
    ASSERT(elements.size() == 2);
    auto key = trimRight(elements[0]);
    auto value = trimLeft(elements[1]);
    j[key] = value;
  }
}

void to_json(json& j, const SegmentIdentificationHeader& h) {
  j = {
      {"ImageIdentifier", h.imageIdentifier},
      {"SegmentNumber", h.segmentNumber},
      {"SegmentStartColumn", h.segmentStartColumn},
      {"SegmentStartLine", h.segmentStartLine},
      {"MaxSegment", h.maxSegment},
      {"MaxColumn", h.maxColumn},
      {"MaxLine", h.maxLine},
  };
}

void to_json(json& j, const NOAALRITHeader& h) {
  // Signature field has 4 bytes, but the string may be shorter.
  // Trim to substring excluding first NUL, if applicable.
  const auto raw = std::string(h.agencySignature, sizeof(h.agencySignature));
  const auto agencySignature = raw.substr(0, raw.find('\0'));
  j = {
      {"AgencySignature", agencySignature},
      {"ProductID", int(h.productID)},
      {"ProductSubID", int(h.productSubID)},
      {"Parameter", int(h.parameter)},
      {"NOAASpecificCompression", int(h.noaaSpecificCompression)},
  };
}

void to_json(json& j, const RiceCompressionHeader& h) {
  j = {
      {"Flags", int(h.flags)},
      {"PixelsPerBlock", int(h.pixelsPerBlock)},
      {"ScanLinesPerPacket", int(h.scanLinesPerPacket)},
  };
}

void to_json(json& j, const DCSFileNameHeader& h) {
  j = h.fileName;
}

#define HEADER(key, klass)                            \
  case klass::CODE:                                   \
    obj.emplace(key, getHeader<klass>(b, it.second)); \
    break;

json toJSON(const Buffer& b, const HeaderMap& m) {
  json obj(json::value_t::object);

  // Add every LRIT header as a JSON object value in the return object.
  for (const auto& it : m) {
    switch (it.first) {
      HEADER("Primary", PrimaryHeader);
      HEADER("ImageStructure", ImageStructureHeader);
      HEADER("ImageNavigation", ImageNavigationHeader);
      HEADER("ImageDataFunction", ImageDataFunctionHeader);
      HEADER("Annotation", AnnotationHeader);
      HEADER("TimeStamp", TimeStampHeader);
      HEADER("AncillaryText", AncillaryTextHeader);
      HEADER("SegmentIdentification", SegmentIdentificationHeader);
      HEADER("NOAALRIT", NOAALRITHeader);
      HEADER("RiceCompression", RiceCompressionHeader);
      HEADER("DCSFileName", DCSFileNameHeader);
    }
  }

  return obj;
}

json toJSON(const File& f) {
  auto b = f.getHeaderBuffer();
  auto m = f.getHeaderMap();
  return toJSON(b, m);
}

} // namespace lrit
