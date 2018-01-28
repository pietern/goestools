#pragma once

#include <unordered_map>

#include "config.h"
#include "handler.h"
#include "image.h"

class GOES16ImageHandler : public Handler {
public:
  explicit GOES16ImageHandler(const Config::Handler& config);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  // The LRIT image files from GOES-16 contain key/value pairs in
  // the ancillary text header. This struct is the interpreted version
  // of that header.
  struct Details {
    struct timespec frameStart;
    std::string satellite;
    std::string instrument;
    Image::Channel channel;
    std::string imagingMode;
    Image::Region region;
    std::string resolution;
    bool segmented;
  };

  Details loadDetails(const lrit::File& f);

  std::string getBasename(const lrit::File& f) const;

  Config::Handler config_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;
  using SegmentsMap = std::map<uint16_t, SegmentVector>;

  // Maintain a map of channel to image identifier to list of segments.
  // Mapping by channel (and assuming the feed always sends images
  // for a single region sequentially), means we can detect missing segments.
  std::unordered_map<std::string, SegmentsMap> segments_;
};
