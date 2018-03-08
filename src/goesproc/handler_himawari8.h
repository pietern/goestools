#pragma once

#include <unordered_map>

#include "config.h"
#include "file_writer.h"
#include "handler.h"
#include "image.h"

class Himawari8ImageHandler : public Handler {
public:
  explicit Himawari8ImageHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  std::string getBasename(const lrit::File& f) const;

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;

  // Maintain a map of image identifier to list of segments.
  std::unordered_map<std::string, SegmentVector> segments_;
};
