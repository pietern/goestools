#pragma once

#include "config.h"
#include "file_writer.h"
#include "handler.h"

class NWSImageHandler : public Handler {
public:
  explicit NWSImageHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  std::string getBasename(const lrit::File& f) const;

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
};
