#include <iostream>
#include <memory>

#include "assembler/vcdu.h"

#include "lib/file_reader.h"
#include "lib/file_writer.h"
#include "lib/nanomsg_reader.h"
#include "options.h"

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);

  // Create packet reader depending on options
  std::unique_ptr<PacketReader> reader;
  if (!opts.publisher.empty()) {
    reader = std::make_unique<NanomsgReader>(opts.publisher);
  } else if (!opts.files.empty()) {
    reader = std::make_unique<FileReader>(opts.files);
  } else {
    std::cerr << "No input specified" << std::endl;
    return 1;
  }

  // Create file writer for current directory
  std::unique_ptr<PacketWriter> writer;
  writer = std::make_unique<FileWriter>(".");

  std::array<uint8_t, 892> buf;
  while (reader->nextPacket(buf)) {
    // Filter by Virtual Channel ID if specified
    if (!opts.vcids.empty()) {
      VCDU vcdu(buf);
      if (opts.vcids.find(vcdu.getVCID()) == opts.vcids.end()) {
        continue;
      }
    }

    writer->write(buf, time(0));
  }
}
