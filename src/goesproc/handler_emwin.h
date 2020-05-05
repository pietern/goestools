#pragma once

#include "config.h"
#include "file_writer.h"
#include "handler.h"

class EMWINHandler : public Handler {
public:
  explicit EMWINHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  bool extractTimeStamp(
      const std::string& text,
      struct timespec& ts) const;

  bool extractAWIPS(
      const std::string& text,
      struct AWIPS& out) const;

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
};
