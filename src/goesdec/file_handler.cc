#include "file_handler.h"

#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>

#include <lib/lrit.h>

FileHandler::FileHandler(const std::string& dir)
  : dir_(dir) {
}

void FileHandler::handle(std::unique_ptr<SessionPDU> spdu) {
  std::string path;
  auto primary = spdu->getHeader<LRIT::PrimaryHeader>();
  auto annotation = spdu->getHeader<LRIT::AnnotationHeader>();
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
  }

  save(*spdu, dir_ + "/" + path + "/" + annotation.text);
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
