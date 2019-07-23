#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <util/fs.h>

#include "lib/file_reader.h"
#include "lib/nanomsg_reader.h"

#include "config.h"
#include "handler_emwin.h"
#include "handler_goesn.h"
#include "handler_goesr.h"
#include "handler_himawari8.h"
#include "handler_nws_image.h"
#include "handler_nws_text.h"
#include "handler_text.h"
#include "options.h"

#include "lrit_processor.h"
#include "packet_processor.h"

using namespace util;

int main(int argc, char** argv) {
  // Dealing with time zones is a PITA even if you only care about UTC.
  // Since this is not a library we can get away with the following...
  setenv("TZ", "", 1);

  auto opts = parseOptions(argc, argv);
  auto config = Config::load(opts.config);
  if (!config.ok) {
    std::cerr << "Invalid configuration: " << config.error << std::endl;
    exit(1);
  }

  // Make sure output directory exists
  mkdirp(opts.out);

  // Handlers share a file writer instance
  auto fileWriter = std::make_shared<FileWriter>(opts.out);
  if (opts.force) {
    fileWriter->setForce(true);
  }

  // Construct list of file handlers
  std::vector<std::unique_ptr<Handler> > handlers;
  for (const auto& handler: config.handlers) {
    if (handler.type == "image") {
      if (handler.origin == "goes16") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new GOESRImageHandler(handler, fileWriter)));
      } else if (handler.origin == "goes17") {
          handlers.push_back(
            std::unique_ptr<Handler>(
              new GOESRImageHandler(handler, fileWriter)));
      } else if (handler.origin == "nws") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new NWSImageHandler(handler, fileWriter)));
      } else if (handler.origin == "himawari8") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new Himawari8ImageHandler(handler, fileWriter)));
      } else if (handler.origin == "goes13") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new GOESNImageHandler(handler, fileWriter)));
      } else if (handler.origin == "goes15") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new GOESNImageHandler(handler, fileWriter)));
      } else {
        std::cerr << "Invalid image handler origin: " << handler.origin << std::endl;
        exit(1);
      }
    } else if (handler.type == "emwin") {
      handlers.push_back(
        std::unique_ptr<Handler>(
          new EMWINHandler(handler, fileWriter)));
    } else if (handler.type == "dcs") {
      // TODO
    } else if (handler.type == "text") {
      if (handler.origin == "nws") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new NWSTextHandler(handler, fileWriter)));
      } else if (handler.origin == "other") {
          handlers.push_back(
            std::unique_ptr<Handler>(
              new TextHandler(handler, fileWriter)));
      } else {
        std::cerr << "Invalid text handler product: " << handler.origin << std::endl;
        exit(1);
      }
    } else {
      std::cerr << "Invalid handler type: " << handler.type << std::endl;
      exit(1);
    }
  }

  if (opts.mode == ProcessMode::PACKET) {
    PacketProcessor p(std::move(handlers));
    std::unique_ptr<PacketReader> reader;

    // Either use subscriber or read packets from files
    if (opts.subscribe.size() > 0) {
      reader = std::make_unique<NanomsgReader>(opts.subscribe);
    } else {
      reader = std::make_unique<FileReader>(opts.paths);
    }

    // Run in verbose mode when stdout is a TTY.
    bool verbose = isatty(fileno(stdout));
    p.run(reader, verbose);
  }

  if (opts.mode == ProcessMode::LRIT) {
    LRITProcessor p(std::move(handlers));
    p.run(argc, argv);
  }
}
