#include <assert.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <memory>

#include "packetizer.h"
#include "reader.h"
#include "vcdu.h"

// Read from file descriptor.
// Stores time when most recent read completed.
// This is used to name output files.
class FileReader : public Reader {
public:
  FileReader(int fd) :
    fd_(fd) {}

  time_t lastRead() const {
    return t_;
  }

  virtual size_t read(void* buf, size_t count) {
    size_t nread = 0;
    while (nread < count) {
      auto rv = ::read(fd_, ((char*) buf) + nread, count - nread);
      // Terminate on error
      if (rv < 0) {
        perror("read");
        exit(1);
      }
      // Return on EOF
      if (rv == 0) {
        return nread;
      }
      nread += rv;
    }
    t_ = time(0);
    return nread;
  }

private:
  int fd_;
  time_t t_;
};

class FileWriter {
public:
  explicit FileWriter(std::string path)
    : path_(path) {
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
      of_.open(fileName_, std::ofstream::out);
      assert(of_.good());

      std::cout
        << "Writing to file: "
        << fileName_
        << std::endl;
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
  auto reader = std::make_shared<FileReader>(0);
  auto writer = std::make_shared<FileWriter>(".");
  Packetizer p(reader);
  std::array<uint8_t, 892> buf;
  for (;;) {
    auto ok = p.nextPacket(buf, nullptr);
    if (!ok) {
      break;
    }

    writer->write(buf, reader->lastRead());
  }
}
