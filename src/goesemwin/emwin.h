#pragma once

#include <unordered_map>
#include <vector>

#include "qbt.h"

namespace emwin {

class File {
public:
  File(std::vector<qbt::Packet> packets) : packets_(packets) {
    if (packets.empty()) {
      throw std::runtime_error("no packets");
    }
  }

  std::string filename() const {
    return packets_.front().filename();
  }

  std::string extension() const;

  std::vector<uint8_t> data() const;

protected:
  std::vector<qbt::Packet> packets_;
};

class Assembler {
public:
  std::unique_ptr<File> process(qbt::Packet p);

protected:
  std::unordered_map<std::string, std::vector<qbt::Packet>> pending_;
};

} // namespace emwin
