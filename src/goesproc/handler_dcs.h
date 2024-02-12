#pragma once

#include "config.h"
#include "file_writer.h"
#include "handler.h"
#include "types.h"

#include "dcs/dcs.h"

class DCSTextHandler : public Handler {
public:
  explicit DCSTextHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
};
