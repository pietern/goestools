#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "assembler/assembler.h"
#include "lrit/file.h"

#include "config.h"
#include "handler_goes16.h"
#include "handler_himawari8.h"
#include "handler_nws.h"

class Processor {
public:
  explicit Processor(std::vector<std::unique_ptr<Handler> > handlers)
    : handlers_(std::move(handlers)) {
  }

  void run(std::istream& is);

protected:
  std::vector<std::unique_ptr<Handler> > handlers_;
  assembler::Assembler assembler_;
};

void Processor::run(std::istream& is) {
  std::array<uint8_t, 892> buf;

  // Keep reading until EOF (supposedly indefinitely)
  for (;;) {
    is.read((char*) buf.data(), buf.size());
    if (!is.good() || is.eof()) {
      break;
    }

    auto spdus = assembler_.process(buf);
    for (auto& spdu : spdus) {
      auto type = spdu->getHeader<lrit::PrimaryHeader>().fileType;

      // Only deal with images for now
      if (type != 0) {
        continue;
      }

      auto file = std::make_shared<lrit::File>(spdu->get());
      for (auto& handler : handlers_) {
        handler->handle(file);
      }
    }
  }
}

int main(int argc, char** argv) {
  // Dealing with time zones is a PITA even if you only care about UTC.
  // Since this is not a library we can get away with the following...
  setenv("TZ", "", 1);

  // Load configuration
  auto config = Config::load("./goesproc.conf");
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

  Processor p(std::move(handlers));
  p.run(std::cin);
}
