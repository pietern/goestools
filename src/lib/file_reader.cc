#include "file_reader.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

FileReader::FileReader(const std::vector<std::string>& files)
    : files_(files) {
}

FileReader::~FileReader() {
}

bool FileReader::nextPacket(std::array<uint8_t, 892>& out) {
  for (;;) {
    // Make sure the ifstream is OK
    if (!ifs_.good() || ifs_.eof()) {
      if (files_.size() == 0) {
        return false;
      }

      // Open next file
      std::cout << "Reading: " << files_.front() << std::endl;
      ifs_.close();
      ifs_.open(files_.front());
      if (!ifs_.good()) {
        std::stringstream ss;
        ss << "Unable to open " << files_.front();
        throw std::runtime_error(ss.str());
      }
      files_.erase(files_.begin());
    }

    ifs_.read((char*) out.data(), out.size());
    if (ifs_.eof()) {
      continue;
    }

    return true;
  }
}
