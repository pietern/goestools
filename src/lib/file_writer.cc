#include "file_writer.h"

#include <array>
#include <cstring>
#include <iostream>

FileWriter::FileWriter(const std::string& path) : path_(path) {
  fileName_ = "";
  fileTime_ = 0;
}

FileWriter::~FileWriter() {
}

void FileWriter::write(const std::array<uint8_t, 892>& buf, time_t t) {
  // Round time down to 5 minute boundary
  t = t - (t % 300);

  // Open new file if necessary
  if (t != fileTime_) {
    if (of_.good()) {
      of_.close();
    }

    fileName_ = timeToFileName(t);
    fileTime_ = t;
    of_.open(fileName_, std::ofstream::out | std::ofstream::app);
    if (!of_.good()) {
      std::cout
        << "Unable to open file: "
        << fileName_
        << " (" << strerror(errno) << ")"
        << std::endl;
    } else {
      std::cout
        << "Writing to file: "
        << fileName_
        << std::endl;
    }
  }

  of_.write((const char*) buf.data(), buf.size());
}

std::string FileWriter::timeToFileName(time_t t) {
  std::array<char, 256> tsbuf;
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "packets-%FT%TZ.raw",
    gmtime(&t));
  return path_ + "/" + std::string(tsbuf.data(), len);
}
