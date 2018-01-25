#include "file.h"

#include <array>

#include <time.h>

namespace lrit {

File::File(const std::string& file)
  : file_(file) {
  std::ifstream ifs(file_.c_str());
  assert(ifs);

  // First 16 bytes hold the primary header
  header_.resize(16);
  ifs.read(reinterpret_cast<char*>(&header_[0]), header_.size());
  assert(ifs);

  // Parse primary header
  ph_ = lrit::getHeader<lrit::PrimaryHeader>(header_, 0);

  // Read remaining headers
  header_.resize(ph_.totalHeaderLength);
  ifs.read(reinterpret_cast<char*>(&header_[16]), ph_.totalHeaderLength - 16);
  assert(ifs);

  // Build header map
  m_ = lrit::getHeaderMap(header_);
}

std::string File::getTime() const {
  std::array<char, 128> tsbuf;
  auto ts = getHeader<lrit::TimeStampHeader>().getUnix();
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%Y%d%m-%H%M%S",
    gmtime(&ts.tv_sec));
  return std::string(tsbuf.data(), len);
}

std::unique_ptr<std::ifstream> File::getData() const {
  auto ifs = std::make_unique<std::ifstream>(file_.c_str());
  assert(*ifs);
  ifs->seekg(ph_.totalHeaderLength);
  assert(*ifs);

  // Because of a bug in goesdec, LRIT image files that used
  // compression have an initial bogus line, followed by the real
  // image. Detect if this is one of those files and ignore the bogus
  // line if so. See 2bd11f16 for more info.
  if (ph_.fileType == 0) {
    // Number of bytes remaining in file
    std::streampos fpos1 = ifs->tellg();
    ifs->seekg(0, std::ios::end);
    std::streampos fpos2 = ifs->tellg();
    ifs->seekg(fpos1);
    // Seek to first byte beyond bogus line
    // Round up (hence the + 7) so that images with 1 bit per pixel
    // and a number of pixels not equal to a power of 8 are not
    // accidentally made 1 byte shorter than expected.
    auto delta = (fpos2 - fpos1) - ((int) ((ph_.dataLength + 7) / 8));
    if (delta > 0) {
      ifs->seekg(delta, ifs->cur);
      assert(*ifs);
    }
  }

  return ifs;
}

std::vector<char> File::read() const {
  std::vector<char> out;

  auto ifs = getData();
  auto bytes = (int) ((ph_.dataLength + 7) / 8);
  out.resize(bytes);
  ifs->read(out.data(), out.size());
  return out;
}

} // namespace lrit
