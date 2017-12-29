#include "sha_handler.h"

#include <iostream>

#include <openssl/sha.h>

void SHAHandler::handle(std::unique_ptr<SessionPDU> spdu) {
  auto primary = spdu->getHeader<LRIT::PrimaryHeader>();
  const auto& buf = spdu->get();
  const unsigned char* ptr = buf.data();
  const size_t len = buf.size();

  // Compute SHA1
  std::string digest;
  {
    std::array<unsigned char, 20> raw;
    std::array<char, 41> buf;
    SHA1(ptr + primary.totalHeaderLength, len - primary.totalHeaderLength, raw.data());

    // Convert raw digest into human readable form
    size_t len = 0;
    for (const auto& byte : raw) {
      len += snprintf(buf.data() + len, buf.size() - len, "%02x", byte);
    }

    // This routine should have written 40 characters
    assert(len == 40);
    digest = std::string(buf.data(), len);
  }

  // Pull file name from header
  auto annotation = spdu->getHeader<LRIT::AnnotationHeader>();
  std::string fileName = annotation.text;

  // Pull time stamp from header (if applicable)
  std::string timeStamp;
  if (spdu->hasHeader<LRIT::TimeStampHeader>()) {
    std::array<char, 128> tsbuf;
    auto h = spdu->getHeader<LRIT::TimeStampHeader>();
    auto ts = h.getUnix();
    auto len = strftime(
      tsbuf.data(),
      tsbuf.size(),
      "%Y-%m-%dT%H:%M:%SZ",
      gmtime(&ts.tv_sec));
    timeStamp = std::string(tsbuf.data(), len);
  }

  std::cout
    << fileName << ","
    << timeStamp << ","
    << digest
    << std::endl;
}
