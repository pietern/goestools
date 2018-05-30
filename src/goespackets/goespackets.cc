#include <iostream>
#include <memory>

#include "assembler/vcdu.h"

#include "lib/file_reader.h"
#include "lib/file_writer.h"
#include "lib/nanomsg_reader.h"
#include "lib/nanomsg_writer.h"
#include "options.h"

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);

  // Create packet reader depending on options
  std::unique_ptr<PacketReader> reader;
  if (!opts.subscribe.empty()) {
    reader = std::make_unique<NanomsgReader>(opts.subscribe);
  } else if (!opts.files.empty()) {
    reader = std::make_unique<FileReader>(opts.files);
  } else {
    std::cerr << "No input specified" << std::endl;
    return 1;
  }

  // Create file writer for current directory
  std::vector<std::unique_ptr<PacketWriter>> writers;
  if (opts.record) {
    writers.push_back(std::make_unique<FileWriter>(opts.filename));
  }
  if (!opts.publish.empty()) {
    writers.push_back(std::make_unique<NanomsgWriter>(opts.publish));
  }

  std::array<uint8_t, 892> buf;
  while (reader->nextPacket(buf)) {
    // Filter by Virtual Channel ID if specified
    if (!opts.vcids.empty()) {
      VCDU vcdu(buf);
      if (opts.vcids.find(vcdu.getVCID()) == opts.vcids.end()) {
        continue;
      }
    }

    for (auto& writer : writers) {
      writer->write(buf, time(0));
    }
  }
}
