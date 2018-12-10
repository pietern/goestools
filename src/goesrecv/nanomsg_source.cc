#include "nanomsg_source.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

std::unique_ptr<Nanomsg> Nanomsg::open(const Config& config) {
  int rv;

  auto fd = nn_socket(AF_SP, NN_SUB);
  if (fd < 0) {
    std::stringstream ss;
    ss << "nn_socket: " << nn_strerror(nn_errno());
    throw std::runtime_error(ss.str());
  }

  const char* url = config.nanomsg.connect.c_str();
  rv = nn_connect(fd, url);
  if (rv < 0) {
    nn_close(fd);
    std::stringstream ss;
    ss << "nn_connect: " << nn_strerror(nn_errno());
    ss << " (" << url << ")";
    throw std::runtime_error(ss.str());
  }

  int size = config.nanomsg.receiveBuffer;
  if (size > 0) {
    rv = nn_setsockopt(fd, NN_SOL_SOCKET, NN_SNDBUF, &size, sizeof(size));
    if (rv < 0) {
      nn_close(fd);
      std::stringstream ss;
      ss << "nn_setsockopt: " << nn_strerror(nn_errno());
      throw std::runtime_error(ss.str());
    }
  }

  rv = nn_setsockopt(fd, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  if (rv < 0) {
    nn_close(fd);
    std::stringstream ss;
    ss << "nn_setsockopt: " << nn_strerror(nn_errno());
    ss << " (" << url << ")";
    throw std::runtime_error(ss.str());
  }

  return std::make_unique<Nanomsg>(fd);
}

Nanomsg::Nanomsg(int fd) : fd_(fd) {
}

Nanomsg::~Nanomsg() {
}

void Nanomsg::setSampleRate(uint32_t sampleRate) {
  sampleRate_ = sampleRate;
}

uint32_t Nanomsg::getSampleRate() const {
  return sampleRate_;
}

void Nanomsg::loop() {
  void* buf = nullptr;
  int nbytes;
  for (;;) {
    nbytes = nn_recv(fd_, &buf, NN_MSG, 0);
    if (nbytes <= 0) {
      return;
    }

    uint32_t nsamples = nbytes / 2;
    int8_t* fi = (int8_t*) buf;

    // Expect multiple of 4
    ASSERT((nsamples & 0x3) == 0);

    // Grab buffer from queue
    auto out = queue_->popForWrite();
    out->resize(nsamples);

    // Convert to std::complex<float>
    auto fo = out->data();
    for (uint32_t i = 0; i < nsamples; i++) {
      fo[i].real((float) fi[i*2+0] / 127.0f);
      fo[i].imag((float) fi[i*2+1] / 127.0f);
    }

    // Processed samples; free nanomsg buffer
    nn_freemsg(buf);

    // Publish output if applicable
    if (samplePublisher_) {
      samplePublisher_->publish(*out);
    }

    // Return buffer to queue
    queue_->pushWrite(std::move(out));
  }
}

void Nanomsg::start(const std::shared_ptr<Queue<Samples> >& queue) {
  queue_ = queue;
  thread_ = std::thread(&Nanomsg::loop, this);
#ifdef __APPLE__
  pthread_setname_np("nanomsg");
#else
  pthread_setname_np(thread_.native_handle(), "nanomsg");
#endif
}

void Nanomsg::stop() {
  nn_close(fd_);

  // Wait for thread to terminate
  thread_.join();

  // Close queue to signal downstream
  queue_->close();

  // Clear reference to queue
  queue_.reset();
}
