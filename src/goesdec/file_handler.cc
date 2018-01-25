#include "file_handler.h"

#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "lrit/lrit.h"

namespace {

std::vector<std::string> split(const std::string& in) {
  std::stringstream ss(in);
  std::vector<std::string> out;
  std::string item;
  while (std::getline(ss, item, '-')) {
    out.push_back(item);
  }
  return out;
}

}

FileHandler::FileHandler(const std::string& dir)
  : dir_(dir) {
}

void FileHandler::handle(std::unique_ptr<SessionPDU> spdu) {
  std::string path;

  // Ignore files without annotation header
  if (!spdu->hasHeader<LRIT::AnnotationHeader>()) {
    return;
  }

  auto primary = spdu->getHeader<LRIT::PrimaryHeader>();
  auto annotation = spdu->getHeader<LRIT::AnnotationHeader>();
  auto fileName = annotation.text;
  switch (primary.fileType) {
  case 0:
    path = "images";
    break;
  case 1:
    path = "messages";
    break;
  case 2:
    path = "text";
    break;
  case 130:
    path = "dcs";
    break;
  default:
    std::cerr
      << "Don't know what to do for file with type " << int(primary.fileType)
      << " (name: " << annotation.text << ")";
  }

  // Per http://www.noaasis.noaa.gov/LRIT/pdf-files/LRIT_receiver-specs.pdf,
  // Table 4, every file has a NOAA LRIT header.
  auto lrit = spdu->getHeader<LRIT::NOAALRITHeader>();
  if (primary.fileType == 0) {
    switch (lrit.productID) {
    case 1:
      path += "/admin";
      break;
    case 2:
      path += "/bulletin";
      break;
    case 3:
      path += "/gms";
      break;
    case 4:
      path += "/meteosat";
      break;
    case 5:
      path += "/noaa-16";
      break;
    case 6:
      path += "/nws";
      break;
    case 7:
      path += "/goes";
      break;
    case 8:
      path += "/dcs";
      break;
    case 13:
      path += "/goes13";
      break;
    case 15:
      path += "/goes15";
      break;
    case 16:
      path += "/goes16";
      break;
    case 43:
      path += "/himawari8";
      break;
    default:
      std::cerr << "Unhandled productID: " << lrit.productID << std::endl;
    }

    // Special case GOES
    if (lrit.productID == 7 || lrit.productID == 13 || lrit.productID == 15) {
      if (lrit.productSubID % 10 == 1) {
        path += "/fd";
      } else if (lrit.productSubID % 10 == 2) {
        path += "/nh";
      } else if (lrit.productSubID % 10 == 3) {
        path += "/sh";
      } else if (lrit.productSubID % 10 == 4) {
        path += "/us";
      } else {
        path += "/special";
      }
    }

    // Special case GOES-16
    if (lrit.productID == 16) {
      auto fileNameParts = split(fileName);
      assert(fileNameParts.size() >= 4);
      if (fileNameParts[2] == "CMIPF") {
        path += "/fd";
      } else if (fileNameParts[2] == "CMIPM1") {
        path += "/m1";
      } else if (fileNameParts[2] == "CMIPM2") {
        path += "/m2";
      } else {
        path += "/unknown";
      }

      // Some image files are segmented, yet have the same annotation
      // text. If we don't do anything, they are all written to the
      // same path. To fix this, we add a file suffix.
      if (spdu->hasHeader<LRIT::SegmentIdentificationHeader>()) {
        auto sih = spdu->getHeader<LRIT::SegmentIdentificationHeader>();
        std::stringstream ss;
        ss << "." << sih.segmentNumber;
        fileName += ss.str();
      }
    }
  }

  auto finalPath = dir_ + "/" + path + "/" + fileName;

  // Ignore EMWIN files and weather reports (on HRIT stream)
  if (primary.fileType == 2 && (lrit.productID == 6 || lrit.productID == 9)) {
    return;
  }

  save(*spdu, finalPath);
}

void FileHandler::save(const SessionPDU& spdu, const std::string& path) {
  std::cerr
    << "Saving " << path
    << " (" << spdu.get().size() << " bytes)"
    << std::endl;

  // Make directories recursively
  createDirectories(path);

  // Save file
  std::ofstream fout(path, std::ofstream::binary);
  const auto& buf = spdu.get();
  fout.write((const char*)buf.data(), buf.size());
  fout.close();
}

void FileHandler::createDirectories(const std::string& path) {
  for (size_t pos = 0; pos < path.size(); pos++) {
    pos = path.find('/', pos);
    if (pos == 0) {
      continue;
    }
    if (pos == std::string::npos) {
      break;
    }
    auto sub = path.substr(0, pos);
    auto rv = mkdir(sub.c_str(), S_IRWXU);
    if (rv == -1 && errno != EEXIST) {
      perror("mkdir");
      assert(rv == 0);
    }
  }
}
