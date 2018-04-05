#pragma once

#include "config.h"
#include "file_writer.h"
#include "handler.h"
#include "types.h"

class TextHandler : public Handler {
public:
  explicit TextHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
};
