#pragma once

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

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
  uint16_t productID_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;
  using SegmentsMap = std::map<uint16_t, SegmentVector>;

  // Maintain a map of channel to image identifier to list of segments.
  // Mapping by channel (and assuming the feed always sends images
  // for a single region sequentially), means we can detect missing segments.
  std::unordered_map<std::string, SegmentsMap> segments_;
};
