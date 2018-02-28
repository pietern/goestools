#include "session_pdu.h"

#include <cassert>
#include <iostream>

namespace assembler {

SessionPDU::SessionPDU(const TransportPDU& tpdu)
  : remainingHeaderBytes_(0),
    lastSequenceCount_(tpdu.sequenceCount()) {
  // First 10 bytes of the first TP_PDU appears to be garbage.
  // Last 2 bytes of every TP_PDU is a CRC.
  auto ok = append(tpdu.data.begin() + 10, tpdu.data.begin() + tpdu.length() - 2);
  assert(ok);
}

bool SessionPDU::canResumeFrom(const TransportPDU& tpdu) const {
  // Can't skip packets if the header is not yet complete.
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

  // Can't skip if the transport PDU is not either a continuation
  // segment, or the last segment for this session PDU.
  auto seq = tpdu.sequenceFlag();
  if (!(seq == 0 || seq == 2)) {
    return false;
  }

  // Can't skip if we need to skip more lines than remaining
  auto ish = getHeader<lrit::ImageStructureHeader>();
  auto remaining = ish.lines - (int) (buf_.size() / szParam_->pixels_per_scanline);
  auto skip = diffWithWrap<16384>(lastSequenceCount_, tpdu.sequenceCount()) - 1;
  if (skip > remaining) {
    return false;
  }

  return true;
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

bool SessionPDU::append(const TransportPDU& tpdu) {
  auto sequenceCount = tpdu.sequenceCount();
  auto skip = diffWithWrap<16384>(lastSequenceCount_, sequenceCount);
  if (skip > 1) {
    // We assume that virtual_channel.cc called the resumable function
    // and it returned that it is OK to skip a number of lines.
    assert(szParam_);

    // Insert black line if there is no contents yet
    auto columns = szParam_->pixels_per_scanline;
    if (buf_.empty()) {
      buf_.insert(buf_.end(), columns, 0);
      skip--;
    }

    // Insert copies of the most recent line
    for (auto i = 0; i < skip; i++) {
      buf_.insert(buf_.end(), buf_.end() - columns, buf_.end());
    }
  }

  lastSequenceCount_ = sequenceCount;
  return append(tpdu.data.begin(), tpdu.data.end() - 2);
}

void SessionPDU::completeHeader() {
  m_ = lrit::getHeaderMap(buf_);

  // File type 0 is image data
  if (ph_.fileType != 0) {
    return;
  }

  // Check compression flag.
  // If it is anything other than "1" we ignore it.
  auto ish = lrit::getHeader<lrit::ImageStructureHeader>(buf_, m_);
  if (ish.compression != 1) {
    return;
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
    completeHeader();
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
  return true;
}

} // namespace assembler
