#pragma once

#include <stdint.h>

#include <istream>
#include <memory>
#include <vector>

// Minimal implementation of a ZIP file reader. This only works for
// ZIP files that contain a single file, as is the case for EMWIN
// files. I have not tested this on other files.
//
// The struct definitions below are taken from the code described at:
// http://codeandlife.com/2014/01/01/unzip-library-for-c/
//
class Zip {
public:
  explicit Zip(std::unique_ptr<std::istream> is);

  std::string fileName() const {
    return fileName_;
  }

  std::vector<char> read() const;

protected:
  std::unique_ptr<std::istream> is_;

  // End Of Central Directory Record
  struct __attribute__ ((__packed__)) eocd {
    uint8_t signature[4];
    uint16_t diskNumber;
    uint16_t centralDirectoryDiskNumber;
    uint16_t numEntriesThisDisk;
    uint16_t numEntries;
    uint32_t centralDirectorySize;
    uint32_t centralDirectoryOffset;
    uint16_t zipCommentLength;
  };

  eocd eocd_;

  // Central Directory File Header
  struct __attribute__ ((__packed__)) cdfh {
    uint8_t signature[4];
    uint16_t versionMadeBy;
    uint16_t versionNeededToExtract;
    uint16_t generalPurposeBitFlag;
    uint16_t compressionMethod;
    uint16_t lastModFileTime;
    uint16_t lastModFileDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
    uint16_t fileCommentLength;
    uint16_t diskNumberStart;
    uint16_t internalFileAttributes;
    uint32_t externalFileAttributes;
    uint32_t relativeOffsetOflocalHeader;
  };

  cdfh cdfh_;

  // Local File Header
  struct __attribute__ ((__packed__)) lfh {
    uint8_t signature[4];
    uint16_t versionNeededToExtract;
    uint16_t generalPurposeBitFlag;
    uint16_t compressionMethod;
    uint16_t lastModFileTime;
    uint16_t lastModFileDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
  };

  lfh lfh_;

  std::string fileName_;
  std::string extraField_;
};
