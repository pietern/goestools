#pragma once

#include <unordered_map>

#include "config.h"
#include "file_writer.h"
#include "handler.h"
#include "image.h"
#include "types.h"

class Himawari8ImageHandler : public Handler {
public:
  explicit Himawari8ImageHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  std::string getBasename(const lrit::File& f) const;
  struct timespec getTime(const lrit::File& f) const;

  void overlayMaps(const lrit::File& f, cv::Mat& mat);

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;

  // Maintain a map of region and channel to list of segments.
  // This assumes that two images for the same region and channel are
  // never sent concurrently, but always in order.
  std::unordered_map<
    SegmentKey,
    SegmentVector,
    SegmentKeyHash> segments_;
};
