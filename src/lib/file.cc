#include "file.h"

#include <array>

#include <time.h>

namespace LRIT {

File::File(const std::string& file)
  : file_(file) {
  std::ifstream ifs(file_.c_str());
  assert(ifs);

  // First 16 bytes hold the primary header
  header_.resize(16);
  ifs.read(reinterpret_cast<char*>(&header_[0]), header_.size());
  assert(ifs);

  // Parse primary header
  ph_ = LRIT::getHeader<LRIT::PrimaryHeader>(header_, 0);

  // Read remaining headers
  header_.resize(ph_.totalHeaderLength);
  ifs.read(reinterpret_cast<char*>(&header_[16]), ph_.totalHeaderLength - 16);
  assert(ifs);

  // Build header map
  m_ = LRIT::getHeaderMap(header_);
}

std::string File::getTime() const {
  std::array<char, 128> tsbuf;
  auto ts = getHeader<LRIT::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y%d%m-%H%M%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}


std::ifstream File::getData() const {
  std::ifstream ifs(file_.c_str());
  assert(ifs);
  ifs.seekg(ph_.totalHeaderLength);
  assert(ifs);
  return std::move(ifs);
}

} // namespace LRIT
