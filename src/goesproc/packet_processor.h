#pragma once

#include <memory>
#include <vector>

#include "assembler/assembler.h"
#include "lib/packet_reader.h"

#include "handler.h"

// Takes a list of files that store LRIT/HRIT VCDUs.
//
// This class assembles packets into files on the fly and
// then feeds them to the handlers.
//
// To run on live data, you can pipe the output of the decoder into
// goesproc and specify /dev/stdin as only file to process.
//
class PacketProcessor {
public:
  explicit PacketProcessor(std::vector<std::unique_ptr<Handler> > handlers);

  void run(std::unique_ptr<PacketReader>& reader, bool verbose);

protected:
  std::vector<std::unique_ptr<Handler> > handlers_;
  assembler::Assembler assembler_;
};
