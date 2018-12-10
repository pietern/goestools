#include "emwin.h"

#include <util/string.h>

using namespace util;

namespace emwin {

std::string File::extension() const {
  auto filename = packets_.front().filename();
  auto pos = filename.find('.');
  return toLower(filename.substr(pos + 1));
}

std::vector<uint8_t> File::data() const {
  std::vector<uint8_t> out;

  // Copy first N-1 packets verbatim
  for (const auto& packet : packets_) {
    auto payload = packet.payload();
    out.insert(out.end(), payload.begin(), payload.end());
  }

  auto begin = out.begin();
  auto end = out.end();
  if (extension() == "zis") {
    // Trim until we have found the ZIP signature
    end -= 22;
    while (end >= begin &&
           !(end[0] == 0x50 &&
             end[1] == 0x4b &&
             end[2] == 0x05 &&
             end[3] == 0x06)) {
      end--;
    }
    end += 22;
  } else {
    // Trim trailing NULs
    end--;
    while (end >= begin && end[0] == 0x00) {
      end--;
    }
    end++;
  }

  out.resize(end - begin);
  return out;
}

std::unique_ptr<File> Assembler::process(qbt::Packet p) {
  auto& vec = pending_[p.filename()];

  // Grab last packet number we have seen for this file
  unsigned long packetNumber = 0;
  if (!vec.empty()) {
    packetNumber = vec.back().packetNumber();
  }

  // Ignore packet if it is not a direct successor to the previous one
  if (p.packetNumber() != (packetNumber + 1)) {
    vec.clear();
    return std::unique_ptr<File>();
  }

  vec.push_back(std::move(p));
  if (vec.size() == vec.back().packetTotal()) {
    return std::make_unique<File>(std::move(vec));
  }

  return std::unique_ptr<File>();
}

} // namespace emwin
