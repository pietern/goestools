#pragma once

#include <tuple>
#include <unordered_map>

#include "config.h"
#include "file_writer.h"
#include "filename.h"
#include "handler.h"
#include "image.h"

class GOES16ImageHandler : public Handler {
public:
  explicit GOES16ImageHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  // The LRIT image files from GOES-16 contain key/value pairs in
  // the ancillary text header. This struct is the interpreted version
  // of that header.
  struct Details {
    struct timespec frameStart;
    Region region;
    Channel channel;
    std::string satellite;
    std::string instrument;
    std::string imagingMode;
    std::string resolution;
    bool segmented;
  };

  void handleImage(
    FilenameBuilder& fb,
    std::unique_ptr<Image> image,
    GOES16ImageHandler::Details details);

  void handleImageForFalseColor(
    FilenameBuilder& fb,
    std::unique_ptr<Image> image,
    GOES16ImageHandler::Details details);

  Details loadDetails(const lrit::File& f);

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;
  using SegmentsMap = std::map<uint16_t, SegmentVector>;

  // Maintain a map of channel to image identifier to list of segments.
  // Mapping by channel (and assuming the feed always sends images
  // for a single region sequentially), means we can detect missing segments.
  std::unordered_map<std::string, SegmentsMap> segments_;

  // To generate false color images we have to keep the image of one channel
  // around while we wait for the other one to be received.
  std::tuple<std::unique_ptr<Image>, Details> tmp_;
};
