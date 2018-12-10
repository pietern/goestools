#include "file_writer.h"

#include <cstring>
#include <iostream>
#include <vector>

#include <util/fs.h>

using namespace util;

FileWriter::FileWriter(const std::string& pattern)
  : pattern_(pattern) {
}

FileWriter::~FileWriter() {
}

void FileWriter::write(const std::array<uint8_t, 892>& buf, time_t t) {
  auto filename = buildFilename(t);

  // Open new file if necessary
  if (filename != currentFilename_) {
    if (of_.good()) {
      of_.close();
    }

    // Optionally mkdir path to new file
    auto rpos = filename.rfind('/');
    if (rpos != std::string::npos) {
      mkdirp(filename.substr(0, rpos));
    }

    // Open new file
    of_.open(filename, std::ofstream::out | std::ofstream::app);
    if (!of_.good()) {
      std::cout
        << "Unable to open file: "
        << filename
        << " (" << strerror(errno) << ")"
        << std::endl;
    } else {
      std::cout
        << "Writing to file: "
        << filename
        << std::endl;
    }

    currentFilename_ = filename;
  }

  of_.write((const char*) buf.data(), buf.size());
}

std::string FileWriter::buildFilename(time_t t) {
  std::vector<char> tsbuf(256);
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    pattern_.c_str(),
    gmtime(&t));
  return std::string(tsbuf.data(), len);
}
