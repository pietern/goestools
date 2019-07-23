#pragma once

#include <tuple>
#include <unordered_map>

#include "config.h"
#include "file_writer.h"
#include "filename.h"
#include "handler.h"
#include "image.h"

class GOESRProduct {
public:
  GOESRProduct() = default;

  explicit GOESRProduct(const std::shared_ptr<const lrit::File>& f);

  const lrit::File& firstFile() const {
    return *files_.front();
  }

  void add(const std::shared_ptr<const lrit::File>& f);

  template <typename H>
  bool hasHeader() const {
    return firstFile().hasHeader<H>();
  }

  template <typename H>
  H getHeader() const {
    return firstFile().getHeader<H>();
  }

  std::map<unsigned int, float> loadImageDataFunction() const;

  FilenameBuilder getFilenameBuilder(const Config::Handler& config) const;

  uint16_t imageIdentifier() const;

  bool isSegmented() const;

  bool isComplete() const;

  std::unique_ptr<Image> getImage(const Config::Handler& config) const;

  bool matchSatelliteID(int satelliteID) const;

  bool matchProduct(const std::vector<std::string>& products) const;

  bool matchRegion(const std::vector<std::string>& regions) const;

  bool matchChannel(const std::vector<std::string>& regions) const;

  SegmentKey generateKey() const;

  const struct timespec getFrameStart() const {
    return frameStart_;
  }

  const Product& getProduct() const {
    return product_;
  }

  const Region& getRegion() const {
    return region_;
  }

  const Channel& getChannel() const {
    return channel_;
  }

protected:
  std::vector<std::shared_ptr<const lrit::File>> files_;

  // GOES-R LRIT image files contain key/value pairs in the ancillary
  // text header. These fields list the set of observed keys, as well
  // as other information describing the nature of the file(s).
  struct timespec frameStart_;
  Product product_;
  Region region_;
  Channel channel_;
  std::string satellite_;
  int satelliteID_{-1};
  std::string instrument_;
  std::string imagingMode_;
  std::string resolution_;
  bool segmented_{false};
};

class GOESRImageHandler : public Handler {
public:
  explicit GOESRImageHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  void handleImage(GOESRProduct product);

  void handleImageForFalseColor(GOESRProduct product);

  void overlayMaps(const GOESRProduct& product, cv::Mat& mat);

  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
  int satelliteID_;

  // Maintain a map of region and channel to list of segments.
  // This assumes that two images for the same region and channel are
  // never sent concurrently, but always in order.
  std::unordered_map<
    SegmentKey,
    GOESRProduct,
    SegmentKeyHash> products_;

  // To generate false color images we have to keep the image of one
  // channel around while we wait for the other one to be received.
  // The first-to-arrive channel is stored in this map and the
  // second-to-arrive channel is handled as it is received. To deal
  // with multiple regions concurrently they are indexed by their
  // region identifier.
  std::unordered_map<std::string, GOESRProduct> falseColor_;
};
