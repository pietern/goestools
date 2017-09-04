#include "reader.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>

// Have to define base class destructor.
Reader::~Reader() {
}

MultiFileReader::MultiFileReader(const std::vector<std::string>& files)
  : files_(files), fd_(-1) {
  std::sort(
    files_.begin(),
    files_.end(),
    [](const std::string& a, const std::string& b) -> bool {
      // Sort by basename of file
      auto apos = a.rfind('/');
      if (apos == std::string::npos) {
        apos = 0;
      } else {
        // Skip over /
        apos += 1;
      }
      auto bpos = b.rfind('/');
      if (bpos == std::string::npos) {
        bpos = 0;
      } else {
        // Skip over /
        bpos += 1;
      }
      return a.substr(apos) < b.substr(bpos);
    });
}

MultiFileReader::~MultiFileReader() {
  if (fd_ != -1) {
    close(fd_);
  }
}

void MultiFileReader::next() {
  if (fd_ != -1) {
    close(fd_);
  }

  // Pop next file off of the list
  if (files_.empty()) {
    std::cerr << "No more files!";
    exit(1);
  }
  current_file_ = files_.front();
  std::cerr << "Reading from: " << current_file_ << std::endl;
  files_.erase(files_.begin());

  // Open new file
  fd_ = open(current_file_.c_str(), O_RDONLY);
  if (fd_ < 0) {
    perror("open");
    exit(1);
  }
}

size_t MultiFileReader::read(void* buf, size_t count) {
  size_t nread = 0;
  int rv;

  if (fd_ == -1) {
    next();
  }

  while (nread < count) {
    rv = ::read(fd_, (char*) buf + nread, count - nread);
    if (rv < 0) {
      perror("read");
      exit(1);
    }

    nread += rv;

    // Move to next file on EOF
    if (rv == 0) {
      next();
    }
  }

  return nread;
}
