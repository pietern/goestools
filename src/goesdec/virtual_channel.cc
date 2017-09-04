#include "virtual_channel.h"

#include <iostream>

VirtualChannel::VirtualChannel(int id)
  : id_(id), n_(-1) {
}

// Combine virtual VirtualChannel packets into transport PDUs.
std::vector<std::unique_ptr<SessionPDU>> VirtualChannel::process(const VCDU& vcdu) {
  std::vector<std::unique_ptr<SessionPDU>> out;
  uint16_t firstHeader;
  size_t pos;

  // Sanity check on VCDU counter (wraps at 2^24)
  if (n_ >= 0) {
    auto skip = diffWithWrap<(1 << 24)>(n_, vcdu.getCounter());

    // Abort processing of pending TP_PDU in case of drop.
    if (skip > 1) {
      std::cerr
        <<  "VC " << id_
        << ": VCDU drop! (lost " << (skip - 1)
        << "; prev: " << n_
        << "; packet: " << vcdu.getCounter()
        << ")"
        << std::endl;
      tpdu_.reset();
    }
  }

  // Update counter with current VCDU
  n_ = vcdu.getCounter();

  // Get iterators to begin/end of the VCDU Data Unit
  auto data = vcdu.data();
  auto len = vcdu.len();

  // Extract "first header pointer" field from M_PDU header
  firstHeader = ((data[0] & 0x7) << 8) | data[1];

  // Skip over M_PDU header
  data += 2;
  len -= 2;

  // Resume extracting a packet if we still have a pointer
  pos = 0;
  if (tpdu_) {
    // Double check that the number of bytes left to read correspond
    // with the first header pointer. The latter takes precedence.
    if (tpdu_->headerComplete()) {
      auto bytesNeeded = tpdu_->length() - tpdu_->data.size();
      // If the first header pointer is 2047, there is no additional
      // header in the VCDU and we can read the entire thing.
      auto bytesAvailable = (firstHeader == 2047) ? len : firstHeader;
      if (firstHeader != 2047 && bytesAvailable < bytesNeeded) {
        std::cerr
          <<  "VC " << id_
          << ": M_SDU continuation failed; "
          << bytesNeeded << " byte(s) needed to complete M_SDU, "
          << bytesAvailable << " byte(s) available"
          << std::endl;
        tpdu_.reset();
      } else {
        pos += tpdu_->read(&data[pos], len - pos);
        if (tpdu_->dataComplete()) {
          process(std::move(tpdu_), out);
        }
      }
    } else {
      pos += tpdu_->read(&data[pos], len - pos);
      if (tpdu_->dataComplete()) {
        process(std::move(tpdu_), out);
      }
    }

    // Return early if we consumed all bytes
    if (pos == len) {
      return out;
    }
  }

  // Must have pointer to first header to continue
  if (firstHeader == 2047) {
    return out;
  }

  // Extract TP_PDUs until there is no more data
  pos = firstHeader;
  while (pos < len) {
    tpdu_ = std::unique_ptr<TransportPDU>(new TransportPDU);
    pos += tpdu_->read(&data[pos], len - pos);
    if (tpdu_->dataComplete()) {
      process(std::move(tpdu_), out);
    }
  }

  return out;
}

void VirtualChannel::process(
    std::unique_ptr<TransportPDU> tpdu,
    std::vector<std::unique_ptr<SessionPDU>>& out) {
  auto apid = tpdu->apid();

  // Ignore stuff on VC 0
  if (id_ == 0) {
    return;
  }

  // Ignore fill packets
  if (apid == 2047) {
    return;
  }

  // Verify CRC is correct
  if (!tpdu->verifyCRC()) {
    std::cerr
      << "VC "
      << id_
      << ": CRC failure; dropping TP_PDU (APID " << apid << ")"
      << std::endl;

    // Clear state for this APID.
    apidSeq_.erase(apid);
    apidSessionPDU_.erase(apid);
    return;
  }

  // The sequence counter is described as (section 6.2.1):
  //
  // > 14-bit packet sequence count, straight sequential count (modulo
  // > 16384) that numbers each source packet generated per APID.
  //
  // Sanity check on this counter to protect against drops.
  //
  auto seq = tpdu->sequenceCount();
  if (apidSeq_.count(apid) > 0) {
    auto skip = diffWithWrap<16384>(apidSeq_[apid], seq);
    if (skip != 1) {
      std::cerr
        << "VC "
        << id_
        << ": Detected TP_PDU drop"
        << " (skipped " << (skip - 1) << " packet(s)"
        << " on APID " << apid
        << "; prev: " << apidSeq_[apid]
        << ", packet: " << seq
        << ")"
        << std::endl;

      // Clear accumulation for this APID
      apidSessionPDU_.erase(apid);
    }
  }
  apidSeq_[apid] = seq;

  // Sequence Flag:
  // - Set to 3 if the user data contains one user data file entirely;
  // - Set to 1 if the user data contains the first segment of one user
  //   data file extending through subsequent packets;
  // - Set to 0 if the user data contains a continuation segment of one
  //   user data file still extending through subsequent packets;
  // - Set to 2 if the user data contains the last segment of a user data
  //   file beginning in an earlier packet.
  auto flag = tpdu->sequenceFlag();
  if (flag == 3 || flag == 1) {
    assert(apidSessionPDU_.count(apid) == 0);

    // Check if this S_PDU is contained in a single TP_PDU
    if (flag == 3) {
      out.push_back(std::make_unique<SessionPDU>(*tpdu));
    } else {
      // Expecting subsequent TP_PDUs to fill this S_PDU
      apidSessionPDU_.insert(
        std::make_pair(apid, std::make_unique<SessionPDU>(*tpdu)));
    }
  } else {
    assert(flag == 0 || flag == 2);

    // Check that this is a valid continuation
    if (apidSessionPDU_.count(apid) == 0) {
      if (false) {
        std::cerr
          << "VC "
          << id_
          << ": Expected to have part of S_PDU for APID "
          << apid
          << std::endl;
      }
    } else {
      // Append data from TP_PDU to temporary buffer
      auto it = apidSessionPDU_.find(apid);
      auto& spdu = it->second;
      if (!spdu->append(*tpdu)) {
        std::cerr
          << "VC "
          << id_
          << ": Unable to append to S_PDU on APID "
          << apid
          << std::endl;
      }
      if (flag == 2) {
        out.push_back(std::move(spdu));
        apidSessionPDU_.erase(apid);
      }
    }
  }
}
