#include "session_pdu.h"

#include <iostream>

namespace assembler {

SessionPDU::SessionPDU(int vcid, int apid)
  : vcid(vcid),
    apid(apid),
    remainingHeaderBytes_(0),
    lastSequenceCount_(0),
    linesDone_(0) {
}

std::string SessionPDU::getName() const {
  if (!hasCompleteHeader()) {
    return "(missing header)";
  }

  if (!hasHeader<lrit::AnnotationHeader>()) {
    return "(missing annotation header)";
  }

  auto ah = getHeader<lrit::AnnotationHeader>();
  return ah.text;
}

void SessionPDU::skipLines(int skip) {
  auto columns = szParam_->pixels_per_scanline;

  // The header must be complete or this function would not be called,
  // so the number of bytes in excess of the header must be greater
  // than or equal to 0.
  auto bytes = buf_.size() - ph_.totalHeaderLength;
  ASSERT(bytes >= 0);

  // Insert black line if there is no contents yet
  if (bytes == 0) {
    buf_.insert(buf_.end(), columns, 0);
    linesDone_++;
    skip--;
  }

  // Insert copies of the most recent line
  for (auto i = 0; i < skip; i++) {
    buf_.insert(buf_.end(), buf_.end() - columns, buf_.end());
    linesDone_++;
  }
}

bool SessionPDU::finish() {
  // Can't finish if the header is not yet complete.
  if (!hasCompleteHeader()) {
    return false;
  }

  // Can't finish if this is not an image
  if (ph_.fileType != 0) {
    return false;
  }

  // Can't finish if we don't know how many pixels to fill in
  if (!szParam_) {
    return false;
  }

  // Skip remainder of image to finish session PDU
  auto ish = getHeader<lrit::ImageStructureHeader>();
  skipLines(ish.lines - linesDone_);
  return true;
}

bool SessionPDU::append(const TransportPDU& tpdu) {
  auto sequenceCount = tpdu.sequenceCount();

  // First 10 bytes of the first TP_PDU appears to be garbage.
  // Last 2 bytes of every TP_PDU is a CRC.
  if (buf_.empty()) {
    auto begin = tpdu.data.begin() + 10;
    auto end = tpdu.data.begin() + tpdu.length() - 2;
    lastSequenceCount_ = sequenceCount;
    return append(begin, end);
  }

  // If one or more packets were skipped (dropped),
  // then we must figure out if it is OK synthesize contents
  // or if we must drop the entire session PDU.
  // This is only the case if the LRIT header was already received
  // and we're dealing with a line-by-line encoded image.
  auto skip = diffWithWrap<16384>(lastSequenceCount_, sequenceCount) - 1;
  if (skip > 0) {
    // Can't skip if the header is not yet complete.
    if (!hasCompleteHeader()) {
      return false;
    }

    // Can't skip if this is not an image
    if (ph_.fileType != 0) {
      return false;
    }

    // Can't skip if we don't know how many pixels to fill in
    if (!szParam_) {
      return false;
    }

    // Can't skip if we need to skip more lines than remaining
    auto ish = getHeader<lrit::ImageStructureHeader>();
    auto remaining = (int) ish.lines - (int) linesDone_;
    if (skip > remaining) {
      return false;
    }

    skipLines(skip);
  }

  lastSequenceCount_ = sequenceCount;
  return append(tpdu.data.begin(), tpdu.data.end() - 2);
}

bool SessionPDU::completeHeader() {
  m_ = lrit::getHeaderMap(buf_);
  if (m_.empty()) {
    return false;
  }

  // File type 0 is image data
  if (ph_.fileType != 0) {
    return true;
  }

  // Check compression flag.
  // If it is anything other than "1" we ignore it.
  auto ish = lrit::getHeader<lrit::ImageStructureHeader>(buf_, m_);
  if (ish.compression != 1) {
    return true;
  }

  // Quote from 5_LRIT_Mission-data.pdf (next to Figure 6)
  //
  //   If the image data is Rice compressed, then the compression
  //   flag, CFLG in the Image Structure Record will be set to a "1".
  //   For uncompressed data, this value will be "0". If the data is
  //   Rice compressed, then a third secondary header must be decoded
  //   to obtain the Rice compression parameters. This is the NOAA
  //   specific Rice Compression Record with a header type equal to
  //   "131" as shown in Figure 6.
  //
  // Therefore, we should now check if this buffer has a Rice
  // compression header and setup the decoder if so.
  //
  if (lrit::hasHeader<lrit::RiceCompressionHeader>(m_)) {
    auto rch = lrit::getHeader<lrit::RiceCompressionHeader>(buf_, m_);
    szParam_.reset(new SZ_com_t);
    szParam_->options_mask = rch.flags | SZ_RAW_OPTION_MASK;
    szParam_->bits_per_pixel = ish.bitsPerPixel;
    szParam_->pixels_per_block = rch.pixelsPerBlock;
    szParam_->pixels_per_scanline = ish.columns;
    szTmp_.resize(szParam_->pixels_per_scanline);
  }

  return true;
}

bool SessionPDU::append(
    std::vector<uint8_t>::const_iterator begin,
    std::vector<uint8_t>::const_iterator end) {
  // Copy primary header first so we can determine total header length
  if (buf_.size() < 16) {
    size_t len = std::min(
      uint32_t(16 - buf_.size()),
      uint32_t(end - begin));
    buf_.insert(buf_.end(), begin, begin + len);
    begin += len;
    if (buf_.size() < 16) {
      return true;
    }
    ph_ = lrit::getHeader<lrit::PrimaryHeader>(buf_, 0);
  }

  // Copy secondary headers verbatim
  if (ph_.totalHeaderLength > buf_.size()) {
    size_t len = std::min(
      uint32_t(ph_.totalHeaderLength - buf_.size()),
      uint32_t(end - begin));
    buf_.insert(buf_.end(), begin, begin + len);
    begin += len;
    if (ph_.totalHeaderLength > buf_.size()) {
      return true;
    }
    if (!completeHeader()) {
      return false;
    }
  }

  // Copy data verbatim if decompression parameters not set
  if (!szParam_) {
    buf_.insert(buf_.end(), begin, end);
    return true;
  }

  // Decompress otherwise
  uint8_t* out = szTmp_.data();
  size_t outLen = szTmp_.size();
  const uint8_t* dataIn = &(*begin);
  size_t dataInLen = end - begin;
  if (dataInLen == 0) {
    return true;
  }

  int rv = SZ_BufftoBuffDecompress(out, &outLen, dataIn, dataInLen, szParam_.get());
  if (rv != AEC_OK) {
    return false;
  }

  // Copy from temporary
  buf_.insert(buf_.end(), szTmp_.begin(), szTmp_.begin() + outLen);
  linesDone_++;
  return true;
}

} // namespace assembler
