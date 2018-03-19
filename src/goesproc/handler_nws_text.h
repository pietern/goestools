#pragma once

#include "config.h"
#include "file_writer.h"
#include "handler.h"
#include "types.h"

class NWSTextHandler : public Handler {
public:
  explicit NWSTextHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  bool extractAWIPS(std::istream& is, struct AWIPS& out);

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
};
