#include "file_source.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::unique_ptr<File> File::open(const Config& config) {
  std::ifstream f(config.file.path);
  if (!f) {
    std::stringstream ss;
    ss << "open: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }

  return std::make_unique<File>(std::move(f));
}

File::File(std::ifstream f) : f_(std::move(f)), stop_(false) {
}

File::~File() {
}

void File::setSampleRate(uint32_t sampleRate) {
  sampleRate_ = sampleRate;
}

uint32_t File::getSampleRate() const {
  return sampleRate_;
}

void File::loop() {
  std::array<float, 128 * 1024> tmp;

  // Determine file size
  f_.seekg(0, f_.end);
  size_t fsize = f_.tellg();
  f_.seekg(0, f_.beg);

  while (!stop_) {
    // Fill temporary buffer
    size_t nread = 0;
    size_t ntotal = (tmp.size() * sizeof(tmp[0]));
    for (;;) {
      // Number of bytes left to read
      size_t nbytes = ntotal - nread;
      // Trim such that we don't hit EOF
      nbytes = std::min(nbytes, fsize - f_.tellg());
      f_.read((char*) tmp.data() + nread, nbytes);
      nread += f_.gcount();
      if (nread == ntotal) {
        break;
      }
      if ((size_t) f_.tellg() == length) {
        f_.seekg(0, f_.beg);
      }
    }

    // Grab buffer from queue
    size_t nsamples = tmp.size() / 2;
    auto out = queue_->popForWrite();
    out->resize(nsamples);

    // Convert to std::complex<float>
    auto fo = out->data();
    for (size_t i = 0; i < nsamples; i++) {
      fo[i].real(tmp[i*2+0]);
      fo[i].imag(tmp[i*2+1]);
    }

    // Publish output if applicable
    if (samplePublisher_) {
      samplePublisher_->publish(*out);
    }

    // Return buffer to queue
    queue_->pushWrite(std::move(out));
  }
}

void File::start(const std::shared_ptr<Queue<Samples> >& queue) {
  queue_ = queue;
  thread_ = std::thread(&File::loop, this);
#ifdef __APPLE__
  pthread_setname_np("file");
#else
  pthread_setname_np(thread_.native_handle(), "file");
#endif
}

void File::stop() {
  stop_.store(true);

  // Wait for thread to terminate
  thread_.join();

  // Close queue to signal downstream
  queue_->close();

  // Clear reference to queue
  queue_.reset();
}
