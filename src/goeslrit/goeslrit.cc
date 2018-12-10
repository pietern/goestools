#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <util/fs.h>

#include "assembler/assembler.h"

#include "lib/file_reader.h"
#include "lib/nanomsg_reader.h"
#include "options.h"

using namespace util;

bool filter(const Options& opts, std::unique_ptr<assembler::SessionPDU>& spdu) {
  // Per http://www.noaasis.noaa.gov/LRIT/pdf-files/LRIT_receiver-specs.pdf,
  // Table 4, every file has a NOAA LRIT header.
  auto ph = spdu->getHeader<lrit::PrimaryHeader>();
  auto nlh = spdu->getHeader<lrit::NOAALRITHeader>();
  if (ph.fileType == 0) {
    return !opts.images;
  }
  if (ph.fileType == 1) {
    return !opts.messages;
  }
  if (ph.fileType == 2) {
    // This may be an EMWIN file.
    if (nlh.productID == 6 || nlh.productID == 9) {
      return !opts.emwin;
    }
    return !opts.text;
  }
  if (ph.fileType == 130) {
    return !opts.dcs;
  }

  std::stringstream ss;
  ss << "Invalid file type: " << ph.fileType;
  throw std::runtime_error(ss.str());
}

std::string filename(std::unique_ptr<assembler::SessionPDU>& spdu) {
  auto out = spdu->getName();
  auto ph = spdu->getHeader<lrit::PrimaryHeader>();
  auto nlh = spdu->getHeader<lrit::NOAALRITHeader>();

  // Special case GOES-16 and GOES-17
  if (ph.fileType == 0 && (nlh.productID == 16 || nlh.productID == 17)) {
    // Some image files are segmented but have the same annotation.
    // To prevent overwriting earlier files, include the segment number.
    if (spdu->hasHeader<lrit::SegmentIdentificationHeader>()) {
      auto sih = spdu->getHeader<lrit::SegmentIdentificationHeader>();
      std::stringstream suffix;
      suffix << "_" << std::setfill('0') << std::setw(3) << sih.segmentNumber;
      out.insert(out.rfind(".lrit"), suffix.str());
    }
  }

  return out;
}

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);

  // Create packet reader depending on options
  std::unique_ptr<PacketReader> reader;
  if (!opts.nanomsg.empty()) {
    reader = std::make_unique<NanomsgReader>(opts.nanomsg);
  } else if (!opts.files.empty()) {
    reader = std::make_unique<FileReader>(opts.files);
  } else {
    std::cerr << "No input specified" << std::endl;
    return 1;
  }

  // Make sure output directory exists
  mkdirp(opts.out);

  // Pass packets to packet assembler
  assembler::Assembler assembler;
  std::array<uint8_t, 892> buf;
  while (reader->nextPacket(buf)) {
    VCDU vcdu(buf);

    // Don't process VCDU if VCID was not specified
    if (!opts.vcids.empty() && opts.vcids.count(vcdu.getVCID()) == 0) {
      continue;
    }

    auto spdus = assembler.process(buf);
    for (auto& spdu : spdus) {

      // Skip stuff without filename
      if (!spdu->hasHeader<lrit::AnnotationHeader>()) {
        continue;
      }

      // Check if we should include this file
      if (filter(opts, spdu)) {
        continue;
      }

      if (opts.dryrun) {
        std::cout << "Writing (dry run): ";
      } else {
        std::cout << "Writing: ";
      }

      const auto name = opts.out + "/" + filename(spdu);
      std::cout << name << " ";

      if (!opts.dryrun) {
        std::ofstream fout(name, std::ofstream::binary);
        const auto& buf = spdu->get();
        fout.write((const char*)buf.data(), buf.size());
        fout.close();
        if (fout.fail()) {
          std::cout << "(" << strerror(errno) << ")" << std::endl;
          continue;
        }
      }

      std::cout << "(" << spdu->size() << " bytes)" << std::endl;
    }
  }
}
