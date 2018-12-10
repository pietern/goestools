#include "zip.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <zlib.h>

#include <util/error.h>

namespace {

uint32_t fromLittleEndian(const uint8_t in[4]) {
  uint32_t out = 0;
  out |= in[0];
  out |= in[1] << 8;
  out |= in[2] << 16;
  out |= in[3] << 24;
  return out;
}

void checkSignature(uint32_t expected, const uint8_t in[4]) {
  auto signature = fromLittleEndian(in);
  if (signature != expected) {
    std::stringstream ss;
    ss.fill('0');
    ss << "Expected signature 0x"
       << std::hex << std::setw(8) << expected << ", "
       << "got signature 0x"
       << std::hex << std::setw(8) << signature;
    throw std::runtime_error(ss.str());
  }
}

std::string loadString(std::unique_ptr<std::istream>& is, size_t len) {
  std::vector<char> tmp(len);
  is->read(tmp.data(), tmp.size());
  return std::string(tmp.data(), tmp.size());
}

} // namespace

Zip::Zip(std::unique_ptr<std::istream> is)
  : is_(std::move(is)) {
  // Load EOCD record
  is_->seekg(-(std::streampos) sizeof(eocd_), is_->end);
  is_->read((char*) &eocd_, sizeof(eocd_));
  ASSERT(is_->good());
  checkSignature(0x06054b50, eocd_.signature);

  // Expect only a single central directory header
  if (eocd_.numEntries != 1) {
    std::stringstream ss;
    ss << "eocd_.numEntries: " << eocd_.numEntries << " != 1";
    throw std::runtime_error(ss.str());
  }

  // Load central directory file header
  is_->seekg(eocd_.centralDirectoryOffset);
  is_->read((char*) &cdfh_, sizeof(cdfh_));
  ASSERT(is_->good());
  checkSignature(0x02014b50, cdfh_.signature);

  // Load local file header
  is_->seekg(cdfh_.relativeOffsetOflocalHeader);
  is_->read((char*) &lfh_, sizeof(lfh_));
  ASSERT(is_->good());
  checkSignature(0x04034b50, lfh_.signature);

  // Load filename
  fileName_ = loadString(is_, lfh_.fileNameLength);
  extraField_ = loadString(is_, lfh_.extraFieldLength);
}

std::vector<char> Zip::read() const {
  std::vector<char> out;

  // Decompress in memory
  out.resize(cdfh_.uncompressedSize);

  // Store
  if (cdfh_.compressionMethod == 0) {
    is_->read(out.data(), out.size());
    return out;
  }

  // Deflate
  if (cdfh_.compressionMethod == 8) {
    std::vector<char> in;
    int rv;

    // Read uncompressed data
    in.resize(cdfh_.compressedSize);
    is_->read(in.data(), in.size());

    // Initialize zlib
    z_stream z;
    z.zalloc = nullptr;
    z.zfree = nullptr;
    z.opaque = nullptr;

    // Use negative window bits to indicate raw data
    rv = inflateInit2(&z, -MAX_WBITS);
    ASSERT(rv == Z_OK);

    // Inflate entire file
    z.avail_in = in.size();
    z.next_in = (unsigned char*) in.data();
    z.avail_out = out.size();
    z.next_out = (unsigned char*) out.data();
    rv = inflate(&z, Z_FINISH);
    ASSERT(rv == Z_STREAM_END);
    ASSERT(z.avail_out == 0);

    // Finalize
    inflateEnd(&z);
    return out;
  }

  throw std::runtime_error("Unknown compression method");
}
