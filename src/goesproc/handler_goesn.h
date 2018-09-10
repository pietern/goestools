#pragma once

#include <tuple>
#include <unordered_map>

#include "config.h"
#include "file_writer.h"
#include "handler.h"
#include "image.h"
#include "types.h"

// Handler for GOES-N series.
// We can treat images from GOES-13 and GOES-15 equally.
class GOESNImageHandler : public Handler {
public:
  explicit GOESNImageHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  // The GOES-N LRIT image files contain key/value pairs in the
  // ancillary text header. A subset is represented in this struct.
  struct Details {
    struct timespec frameStart;
    std::string satellite;
  };

  Details loadDetails(const lrit::File& f);
  Region loadRegion(const lrit::NOAALRITHeader& h) const;
  Channel loadChannel(const lrit::NOAALRITHeader& h) const;

  void overlayMaps(const lrit::File& f, const Area& crop, cv::Mat& mat);

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
  uint16_t productID_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;

  // Maintain a map of region and channel to list of segments.
  // This assumes that two images for the same region and channel are
  // never sent concurrently, but always in order.
  std::unordered_map<
    SegmentKey,
    SegmentVector,
    SegmentKeyHash> segments_;
};
