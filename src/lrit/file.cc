#include "file.h"

#include <array>

#include <time.h>

#include <util/error.h>

namespace lrit {

namespace {

// http://tuttlem.github.io/2014/08/18/getting-istream-to-work-off-a-byte-array.html
class membuf : public std::basic_streambuf<char> {
public:
  membuf(const uint8_t* p, size_t l) {
    setg((char*)p, (char*)p, (char*)p + l);
  }

  pos_type seekpos(
      pos_type sp,
      std::ios_base::openmode which) override {
    return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
  }

  pos_type seekoff(
      off_type off,
      std::ios_base::seekdir dir,
      std::ios_base::openmode which = std::ios_base::in) override {
    if (dir == std::ios_base::cur) {
      gbump(off);
    } else if (dir == std::ios_base::end) {
      setg(eback(), egptr() + off, egptr());
    } else if (dir == std::ios_base::beg) {
      setg(eback(), eback() + off, egptr());
    }
    return gptr() - eback();
  }
};

class memstream : public std::istream {
public:
  memstream(const uint8_t* p, size_t l)
    : std::istream(&buf_),
      buf_(p, l) {
    rdbuf(&buf_);
  }

private:
  membuf buf_;
};

class offsetfilebuf : public std::filebuf {
public:
  void mark() {
    offset_ = seekoff(0, std::ios_base::cur);
  }

  pos_type seekpos(
      pos_type sp,
      std::ios_base::openmode which) override {
    return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
  }

  pos_type seekoff(
      off_type off,
      std::ios_base::seekdir dir,
      std::ios_base::openmode which = std::ios_base::in) override {
    pos_type pos = 0;
    if (dir == std::ios_base::cur) {
      pos = std::filebuf::seekoff(off, dir, which);
    } else if (dir == std::ios_base::end) {
      pos = std::filebuf::seekoff(off, dir, which);
    } else if (dir == std::ios_base::beg) {
      pos = std::filebuf::seekoff(off + offset_, dir, which);
    }
    return pos - offset_;
  }

private:
  off_type offset_ = 0;
};

class offsetifstream : public std::istream {
public:
  offsetifstream(const std::string& path)
    : std::istream(&buf_) {
    buf_.open(path, std::ios::in);
  }

  void mark() {
    buf_.mark();
  }

private:
  offsetfilebuf buf_;
};

} // namespace

File::File(const std::string& file)
  : file_(file) {
  std::ifstream ifs(file_.c_str());
  ASSERT(ifs);

  // First 16 bytes hold the primary header
  header_.resize(16);
  ifs.read(reinterpret_cast<char*>(&header_[0]), header_.size());
  ASSERT(ifs);

  // Parse primary header
  ph_ = lrit::getHeader<lrit::PrimaryHeader>(header_, 0);

  // Read remaining headers
  header_.resize(ph_.totalHeaderLength);
  ifs.read(reinterpret_cast<char*>(&header_[16]), ph_.totalHeaderLength - 16);
  ASSERT(ifs);

  // Build header map
  m_ = lrit::getHeaderMap(header_);
}

File::File(const std::vector<uint8_t>& buf) : buf_(buf) {
  // First 16 bytes hold the primary header
  header_.insert(header_.end(), buf_.begin(), buf_.begin() + 16);

  // Parse primary header
  ph_ = lrit::getHeader<lrit::PrimaryHeader>(header_, 0);

  // Read remaining headers
  header_.insert(
    header_.end(),
    buf_.begin() + 16,
    buf_.begin() + ph_.totalHeaderLength);

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

std::unique_ptr<std::istream> File::getDataFromFile() const {
  auto ifs = std::make_unique<offsetifstream>(file_);
  ASSERT(*ifs);
  ifs->seekg(ph_.totalHeaderLength);
  ASSERT(*ifs);

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
      ASSERT(*ifs);
    }
  }

  // Current position is the start of the data section,
  // and the 0-offset for other code that uses it.
  ifs->mark();

  return std::unique_ptr<std::istream>(ifs.release());
}

std::unique_ptr<std::istream> File::getDataFromBuffer() const {
  auto ms = std::make_unique<memstream>(
    buf_.data() + ph_.totalHeaderLength,
    buf_.size() - ph_.totalHeaderLength);
  return std::unique_ptr<std::istream>(ms.release());
}

std::unique_ptr<std::istream> File::getData() const {
  if (!file_.empty()) {
    return getDataFromFile();
  }
  if (!buf_.empty()) {
    return getDataFromBuffer();
  }
  ERROR("unreachable");
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
