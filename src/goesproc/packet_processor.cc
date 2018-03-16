#include "packet_processor.h"

#include "lrit/file.h"

PacketProcessor::PacketProcessor(std::vector<std::unique_ptr<Handler> > handlers)
    : handlers_(std::move(handlers)) {
}

void PacketProcessor::run(int argc, char** argv) {
  std::vector<std::string> files;
  for (int i = 0; i < argc; i++) {
    files.push_back(argv[i]);
  }

  // Read from stdin if nothing is specified
  if (files.empty()) {
    // This only works on Linux...
    files.push_back("/proc/self/fd/0");
  }

  // This function assumes the files to read are given in
  // chronological order. Bash shell expansion maintains alphabetical
  // order and we assume that alphabetical ordering implies
  // chronological ordering here.
  std::array<uint8_t, 892> buf;
  for (const auto& file : files) {
    std::ifstream ifs(file);
    for (;;) {
      ifs.read((char*) buf.data(), buf.size());
      if (!ifs.good() || ifs.eof()) {
        break;
      }

      auto spdus = assembler_.process(buf);
      for (auto& spdu : spdus) {
        auto file = std::make_shared<lrit::File>(spdu->get());
        for (auto& handler : handlers_) {
          handler->handle(file);
        }
      }
    }
  }
}
