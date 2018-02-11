#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "config.h"
#include "handler_goes16.h"
#include "handler_goesn.h"
#include "handler_himawari8.h"
#include "handler_nws.h"
#include "options.h"

#include "lrit_processor.h"
#include "packet_processor.h"

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

  // Construct list of file handlers
  std::vector<std::unique_ptr<Handler> > handlers;
  for (const auto& handler: config.handlers) {
    if (handler.type == "image") {
      if (handler.product == "goes16") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new GOES16ImageHandler(handler)));
      } else if (handler.product == "nws") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new NWSImageHandler(handler)));
      } else if (handler.product == "himawari8") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new Himawari8ImageHandler(handler)));
      } else if (handler.product == "goes13") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new GOESNImageHandler(handler)));
      } else if (handler.product == "goes15") {
        handlers.push_back(
          std::unique_ptr<Handler>(
            new GOESNImageHandler(handler)));
      } else {
        std::cerr << "Invalid image handler product: " << handler.product << std::endl;
        exit(1);
      }
    } else if (handler.type == "dcs") {
      // TODO
    } else if (handler.type == "text") {
      // TODO
    } else {
      std::cerr << "Invalid handler type: " << handler.type << std::endl;
      exit(1);
    }
  }

  if (opts.mode == ProcessMode::PACKET) {
    PacketProcessor p(std::move(handlers));
    p.run(argc, argv);
  }

  if (opts.mode == ProcessMode::LRIT) {
    LRITProcessor p(std::move(handlers));
    p.run(argc, argv);
  }
}
