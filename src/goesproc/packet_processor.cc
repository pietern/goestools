#include "packet_processor.h"

#include <iomanip>

#include "lrit/file.h"

PacketProcessor::PacketProcessor(std::vector<std::unique_ptr<Handler> > handlers)
    : handlers_(std::move(handlers)) {
}

void PacketProcessor::run(std::unique_ptr<PacketReader>& reader, bool verbose) {
  if (verbose) {
    std::cout
      << "Waiting for first packet..."
      // Carriage return to return to beginning of line
      << "\r"
      // Flush buffers
      << std::flush
      // Erase in line (expected to be buffered)
      << "\033[K";
  }

  std::array<uint8_t, 892> buf;
  while (reader->nextPacket(buf)) {
    if (verbose) {
      VCDU vcdu(buf);
      std::cout
        << "Packet:"
        << " SCID="
        << std::setw(1) << vcdu.getSCID()
        << " VCID="
        << std::setw(2) << vcdu.getVCID()
        << " counter="
        << std::setw(8) << std::setfill('0') << vcdu.getCounter()
        // Carriage return to return to beginning of line
        << "\r"
        // Flush buffers
        << std::flush
        // Erase in line (expected to be buffered)
        << "\033[K";
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
