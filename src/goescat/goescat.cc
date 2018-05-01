#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "assembler/vcdu.h"

#include "lib/file_reader.h"
#include "lib/nanomsg_reader.h"
#include "options.h"

class FileWriter {
public:
  explicit FileWriter(std::string path) : path_(path) {
    fileName_ = "";
    fileTime_ = 0;
  }

  void write(const std::array<uint8_t, 892>& buf, time_t t) {
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

protected:
  std::string path_;
  std::string fileName_;
  time_t fileTime_;
  std::ofstream of_;

  std::string timeToFileName(time_t t) {
    std::array<char, 256> tsbuf;
    auto len = strftime(
      tsbuf.data(),
      tsbuf.size(),
      "packets-%FT%TZ.raw",
      gmtime(&t));
    return path_ + "/" + std::string(tsbuf.data(), len);
  }
};

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);

  // Create packet reader depending on options
  std::unique_ptr<PacketReader> reader;
  if (!opts.publisher.empty()) {
    reader = std::make_unique<NanomsgReader>(opts.publisher);
  } else if (!opts.files.empty()) {
    reader = std::make_unique<FileReader>(opts.files);
  } else {
    std::cerr << "No input specified" << std::endl;
    return 1;
  }

  // Create file writer for current directory
  auto writer = std::make_unique<FileWriter>(".");

  std::array<uint8_t, 892> buf;
  while (reader->nextPacket(buf)) {
    // Filter by Virtual Channel ID if specified
    if (!opts.vcids.empty()) {
      VCDU vcdu(buf);
      if (opts.vcids.find(vcdu.getVCID()) == opts.vcids.end()) {
        continue;
      }
    }

    writer->write(buf, time(0));
  }
}
