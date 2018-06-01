#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "assembler/assembler.h"

#include "lib/file_reader.h"
#include "lib/nanomsg_reader.h"
#include "options.h"

std::string filename(std::unique_ptr<assembler::SessionPDU>& spdu) {
  struct timespec ts;
  auto rv = clock_gettime(CLOCK_REALTIME, &ts);
  assert(rv >= 0);

  // Use 'parameter' field in NOAA LRIT header as counter
  auto nlh = spdu->getHeader<lrit::NOAALRITHeader>();

  // Use filename pattern similar to the one Dartcom uses
  std::array<char, 128> tsbuf;
  auto len = strftime(
    tsbuf.data(),
    tsbuf.size(),
    "%y%m%d_%H%M%S",
    gmtime(&ts.tv_sec));
  len += snprintf(
    tsbuf.data() + len,
    tsbuf.size() - len,
    "_%05d.000",
    nlh.parameter);
  assert(len < tsbuf.size());
  return std::string(tsbuf.data(), len);
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

  // Pass packets to packet assembler
  assembler::Assembler assembler;
  std::array<uint8_t, 892> buf;
  while (reader->nextPacket(buf)) {
    VCDU vcdu(buf);

    // EMWIN packets are sent on GOES-N series VCID 0
    if (vcdu.getVCID() != 0) {
      continue;
    }

    auto spdus = assembler.process(buf);
    for (auto& spdu : spdus) {
      // EMWIN packets have file type 214
      auto ph = spdu->getHeader<lrit::PrimaryHeader>();
      if (ph.fileType != 214) {
        continue;
      }

      // EMWIN packets have product ID 42
      auto nlh = spdu->getHeader<lrit::NOAALRITHeader>();
      if (nlh.productID != 42) {
        continue;
      }

      if (opts.dryrun) {
        std::cout << "Writing (dry run): ";
      } else {
        std::cout << "Writing: ";
      }

      const auto name = filename(spdu);
      std::cout << name << " ";

      const auto& buf = spdu->get();
      auto len = buf.size() - ph.totalHeaderLength;
      if (!opts.dryrun) {
        std::ofstream fout(name, std::ofstream::binary);
        auto ptr = ((const char*) buf.data()) + ph.totalHeaderLength;
        fout.write(ptr, len);
        fout.close();
        if (fout.fail()) {
          std::cout << "(" << strerror(errno) << ")" << std::endl;
          continue;
        }
      }

      std::cout << "(" << len << " bytes)" << std::endl;
    }
  }
}
