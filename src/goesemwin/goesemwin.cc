#include <array>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <util/error.h>
#include <util/string.h>

#include "assembler/assembler.h"
#include "lib/file_reader.h"
#include "lib/nanomsg_reader.h"
#include "lib/zip.h"

#include "emwin.h"
#include "options.h"
#include "qbt.h"

using namespace util;

template<typename T>
std::string filename(const T& t);

template<>
std::string filename(const qbt::Fragment& f) {
  struct timespec ts;
  auto rv = clock_gettime(CLOCK_REALTIME, &ts);
  ASSERT(rv >= 0);

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
    f.counter());
  ASSERT(len < tsbuf.size());
  return std::string(tsbuf.data(), len);
}

template<>
std::string filename(const qbt::Packet& p) {
  std::stringstream ss;
  ss << p.filename();
  ss << ".";
  ss << std::setfill('0') << std::setw(3) << p.packetNumber();
  return ss.str();
}

template<>
std::string filename(const emwin::File& f) {
  return f.filename();
}

template<typename T>
void write(const Options& opts, const T& t) {
  auto data = t.data();
  auto path = opts.out + "/" + filename(t);
  std::cout << "Writing " << path;
  std::ofstream fout(path, std::ofstream::binary);
  fout.write((const char*) &data[0], data.size());
  fout.close();
  if (fout.fail()) {
    std::cout << " (error: " << strerror(errno) << ")";
  }
  std::cout << std::endl;
}

// FragmentReader reads packets, filters EMWIN S_PDUs, and yields
// their associated packet counter and payload.
class FragmentReader {
public:
  FragmentReader(std::unique_ptr<PacketReader> reader)
    : reader_(std::move(reader)) {
  }

  bool next(std::vector<qbt::Fragment>& fragments) {
    fragments.clear();

    std::array<uint8_t, 892> buf;
    while (reader_->nextPacket(buf)) {
      VCDU vcdu(buf);

      // EMWIN packets are sent on GOES-N series VCID 0
      if (vcdu.getVCID() != 0) {
        continue;
      }

      auto spdus = assembler_.process(buf);
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

        // Use 'parameter' field in NOAA LRIT header as counter
        const auto counter = nlh.parameter;
        const auto& payload = spdu->get();
        const auto begin = payload.begin() + ph.totalHeaderLength;
        const auto end = payload.end();
        fragments.push_back(qbt::Fragment(counter, begin, end));
      }

      if (!fragments.empty()) {
        return true;
      }
    }

    return false;
  }

protected:
  std::unique_ptr<PacketReader> reader_;
  assembler::Assembler assembler_;
};

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);

  // Create packet reader depending on options
  std::unique_ptr<PacketReader> packetReader;
  if (!opts.nanomsg.empty()) {
    packetReader = std::make_unique<NanomsgReader>(opts.nanomsg);
  } else if (!opts.files.empty()) {
    packetReader = std::make_unique<FileReader>(opts.files);
  } else {
    std::cerr << "No input specified" << std::endl;
    return 1;
  }

  // Pass packets to packet assembler
  FragmentReader reader(std::move(packetReader));
  qbt::Assembler qbtAssembler;
  emwin::Assembler emwinAssembler;
  std::vector<qbt::Fragment> fragments;
  while (reader.next(fragments)) {
    for (auto& fragment : fragments) {

      // Write raw fragments (the data portion of EMWIN-over-LRIT files)
      if (opts.mode == Mode::RAW) {
        write(opts, fragment);
        continue;
      }

      // Pass fragment to QBT packet assembler
      auto packet = qbtAssembler.process(std::move(fragment));
      if (!packet) {
        continue;
      }

      // Write QBT packet if configured to do so
      if (opts.mode == Mode::QBT) {
        write(opts, *packet);
        continue;
      }

      // Pass packet to EMWIN file assembler
      auto file = emwinAssembler.process(std::move(*packet));
      if (!file) {
        continue;
      }

      // Write EMWIN file if configured to do so
      if (opts.mode == Mode::EMWIN) {
        write(opts, *file);
        continue;
      }
    }
  }
}
