#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "image.h"
#include "options.h"

class ProcessImageData {
public:
  ProcessImageData(Options&& opts, std::vector<Image>&& images) :
      opts_(std::move(opts)),
      images_(std::move(images)) {
    std::set<int> subProducts;
    for (auto& image : images_) {
      subProducts.insert(
        image.getFiles().front().getHeader<LRIT::NOAALRITHeader>().productSubID);
    }
    multiSubProducts_ = (subProducts.size() > 1);
  }

  int run() {
    auto minArea = computeMinArea();
    for (auto& image : images_) {
      auto fileName = image.getSatellite() + "_";

      // If this list contains images from multiple products, the
      // product is omitted from the filename, so the resulting set of
      // files is chronologically ordered by default.
      if (!multiSubProducts_) {
        fileName += image.getProductShort() + "_";
      }
      fileName += image.getChannelShort() + "_";
      fileName += image.getTimeShort();

      // Skip incomplete images
      if (!image.complete()) {
        std::cout
          << "Skipping " << fileName
          << " (have " << int(image.getActualNumberFiles())
          << "/" << int(image.getExpectedNumberFiles())
          << ")"
          << std::endl;
        continue;
      }

      auto im = image.getScaledImage(minArea, opts_.shrink);

      int width = im.cols;
      int height = im.rows;
      int annotationRows = height / 40;
      if (opts_.scale.width > 0 && opts_.scale.height > 0) {
        width = opts_.scale.width;
        height = opts_.scale.height;
        annotationRows = height / 40;

        // Subtract the number of rows used for annotation so that
        // it can be added without growing the image beyond <width>x<height>.
        im = scale(im, width, height - annotationRows);
      }

      im = annotate(image, im, annotationRows);
      fileName += "." + opts_.format;
      std::cout
        << "Writing "
        << fileName
        << " ("
        << im.cols
        << "x"
        << im.rows
        << ")"
        << std::endl;
      cv::imwrite(fileName, im);
    }
    return 0;
  }

  Image::Area computeMinArea() {
    bool initialized = false;
    Image::Area area;
    for (const auto& image : images_) {
      if (!image.complete()) {
        continue;
      }
      if (initialized) {
        area = area.getIntersection(image.getArea());
      } else {
        initialized = true;
        area = image.getArea();
      }
    }
    return area;
  }

  cv::Mat scale(cv::Mat src, int x, int y) {
    float srcRatio = (float)src.cols / (float)src.rows;
    float dstRatio = (float)x / (float)y;
    if (srcRatio < dstRatio) {
      // Destination is wider than source.
      // Need to crop top/bottom.
      cv::Mat tmp(x / srcRatio, x, src.type());
      cv::resize(src, tmp, tmp.size());
      // Compute y offset for crop setting
      auto ydiff = tmp.rows - y;
      auto yoffset = 0;
      switch (opts_.scale.cropHeight) {
      case CropHeight::TOP:
        yoffset = ydiff;
        break;
      case CropHeight::CENTER:
        yoffset = ydiff / 2;
        break;
      case CropHeight::BOTTOM:
        yoffset = 0;
        break;
      }
      return tmp(cv::Rect(0, yoffset, x, y));
    } else if (srcRatio > dstRatio) {
      // Destination is taller than source.
      // Need to crop left/right.
      cv::Mat tmp(y, y * srcRatio, src.type());
      cv::resize(src, tmp, tmp.size());
      // Compute x offset for crop setting
      auto xdiff = tmp.cols - x;
      auto xoffset = 0;
      switch (opts_.scale.cropWidth) {
      case CropWidth::LEFT:
        xoffset = xdiff;
        break;
      case CropWidth::CENTER:
        xoffset = xdiff / 2;
        break;
      case CropWidth::RIGHT:
        xoffset = 0;
        break;
      }
      return tmp(cv::Rect(xoffset, 0, x, y));
    } else {
      // Destination has same aspect ratio as source.
      // Only need to resize.
      cv::Mat tmp(y, x, src.type());
      cv::resize(src, tmp, tmp.size());
      return tmp;
    }
  }

  cv::Mat annotate(const Image& image, cv::Mat in, int topRows) {
    cv::Mat tmp(in.rows + topRows, in.cols, in.type());

    // Copy contents
    auto body = cv::Mat(tmp, cv::Rect(0, topRows, in.cols, in.rows));
    in.copyTo(body);

    // Generate annotation
    auto details = cv::Mat(tmp, cv::Rect(0, 0, in.cols, topRows));
    details.setTo(cv::Scalar(0));

    // Build horizontal segments to write text into
    std::array<cv::Mat, 3> segs;
    int segWidth = in.cols / segs.size();
    for (unsigned i = 0; i < segs.size(); i++) {
      segs[i] = cv::Mat(
        details,
        cv::Rect(i * segWidth, 0, segWidth, topRows));
    }
    putText(segs[0], 0, image.getSatellite() + " " + image.getProductLong());
    putText(segs[1], 1, image.getTimeLong() + " UTC");
    putText(segs[2], 2, image.getChannelLong());
    return tmp;
  }

  void putText(cv::Mat& dst, int align, std::string str) {
    auto face = cv::FONT_HERSHEY_SIMPLEX;
    auto scale = 1.0f;
    int thickness = 1;
    int baseline = 0;

    // Compute scale such that the height of the bounding box
    // is 90% of the height of the target area.
    auto bb = cv::getTextSize(str, face, scale, thickness, &baseline);
    scale *= (float)dst.rows * 0.5f / (float)bb.height;
    bb = cv::getTextSize(str, face, scale, thickness, &baseline);

    cv::Point org;
    auto margin = (dst.rows - bb.height) / 2;
    switch (align) {
    case 0: // Left
      org = cv::Point(margin, (dst.rows + bb.height) / 2);
      break;
    case 1: // Center
      org = cv::Point((dst.cols - bb.width) / 2, (dst.rows + bb.height) / 2);
      break;
    case 2: // Right
      org = cv::Point((dst.cols - bb.width - margin), (dst.rows + bb.height) / 2);
      break;
    }
    cv::Scalar white(255);
    cv::putText(dst, str, org, face, scale, white, thickness);
  }

protected:
  Options opts_;
  std::vector<Image> images_;
  bool multiSubProducts_;
};

int processImageData(Options& opts) {
  auto& files = opts.files;
  auto productID = files.front().getHeader<LRIT::NOAALRITHeader>().productID;
  switch (productID) {
  case 13:
    // GOES-13
    break;
  case 15:
    // GOES-15
    break;
  case 3:
    // GMS
    break;
  case 4:
    // METEOSAT
    break;
  default:
    std::cerr
      << "Image handler for NOAA LRIT Product ID "
      << productID
      << " not implemented..."
      << std::endl;
    return 1;
  }

  // Check we're dealing with files from a single satellite
  for (const auto& f : files) {
    auto lrit = f.getHeader<LRIT::NOAALRITHeader>();
    if (lrit.productID != productID) {
      std::cerr << "Specified images from multiple satellites..." << std::endl;
      return 1;
    }
  }

  std::map<int, std::vector<LRIT::File>> filesByImageID;
  for (const auto& f : files) {
    auto si = f.getHeader<LRIT::SegmentIdentificationHeader>();
    filesByImageID[si.imageIdentifier].push_back(std::move(f));
  }

  std::vector<Image> images;
  for (auto& e : filesByImageID) {
    Image image(e.first, e.second);

    // Filter by channel if channel option is set
    if (!opts.channel.empty() && opts.channel != image.getChannelShort()) {
      continue;
    }

    images.push_back(std::move(image));
  }

  return ProcessImageData(std::move(opts), std::move(images)).run();
}

int processMessages(Options& opts) {
  return 0;
}

int processText(Options& opts) {
  return 0;
}

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);
  switch (opts.fileType) {
  case 0:
    return processImageData(opts);
  case 1:
    return processMessages(opts);
  case 2:
    return processText(opts);
  default:
    std::cerr
      << "No handler for LRIT file type "
      << int(opts.fileType)
      << "..."
      << std::endl;
    exit(1);
  }
}
