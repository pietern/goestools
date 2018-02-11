#pragma once

#include <unordered_map>

#include "config.h"
#include "handler.h"
#include "image.h"

class Himawari8ImageHandler : public Handler {
public:
  explicit Himawari8ImageHandler(const Config::Handler& config);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  std::string getBasename(const lrit::File& f) const;

  Config::Handler config_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;

  // Maintain a map of image identifier to list of segments.
  std::unordered_map<std::string, SegmentVector> segments_;
};
