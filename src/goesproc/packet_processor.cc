#include "packet_processor.h"

#include "lrit/file.h"

PacketProcessor::PacketProcessor(std::vector<std::unique_ptr<Handler> > handlers)
    : handlers_(std::move(handlers)) {
}

void PacketProcessor::run(std::unique_ptr<PacketReader>& reader) {
  std::array<uint8_t, 892> buf;
  while (reader->nextPacket(buf)) {
    auto spdus = assembler_.process(buf);
    for (auto& spdu : spdus) {
      auto file = std::make_shared<lrit::File>(spdu->get());
      for (auto& handler : handlers_) {
        handler->handle(file);
      }
    }
  }
}
