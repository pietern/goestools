#pragma once

#include <tuple>
#include <unordered_map>

#include "config.h"
#include "file_writer.h"
#include "filename.h"
#include "handler.h"
#include "image.h"

class GOESRImageHandler : public Handler {
public:
  explicit GOESRImageHandler(
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
    int satelliteID;
    std::string instrument;
    std::string imagingMode;
    std::string resolution;
    bool segmented;
  };

  // Complete images are passed along together with any information we
  // need from the source LRIT files without holding onto them.
  // For example, the default filename comes from the LRIT header.
  struct Tuple {
    Tuple() {
    }

    Tuple(
      std::unique_ptr<Image> image,
      Details details,
      FilenameBuilder fb)
      : image(std::move(image)),
        details(std::move(details)),
        fb(std::move(fb)) {
    }

    std::unique_ptr<Image> image;
    Details details;
    FilenameBuilder fb;
  };

  void handleImage(Tuple t);

  void handleImageForFalseColor(Tuple t);

  Details loadDetails(const lrit::File& f);

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
  int satelliteID_;

  using SegmentVector = std::vector<std::shared_ptr<const lrit::File>>;

  // Maintain a map of region and channel to list of segments.
  // This assumes that two images for the same region and channel are
  // never sent concurrently, but always in order.
  std::unordered_map<
    SegmentKey,
    SegmentVector,
    SegmentKeyHash> segments_;

  // To generate false color images we have to keep the image of one
  // channel around while we wait for the other one to be received.
  // The first-to-arrive channel is stored in this map and the
  // second-to-arrive channel is handled as it is received. To deal
  // with multiple regions concurrently they are indexed by their
  // region identifier.
  std::unordered_map<std::string, Tuple> falseColor_;
};
