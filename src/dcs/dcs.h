// From  https://dcs1.noaa.gov/documents/HRIT%20DCS%20File%20Format%20Rev1.pdf
#pragma once

#include <unistd.h>

#include <ctime>
#include <string>
#include <vector>
#include <unordered_map>

namespace dcs {

// Header at the beginning of an LRIT DCS file
 
// 3.1 File Header
struct FileHeader {

// 3.1.1 FILE_NAME
  std::string name;

// 3.1.2 FILE_SIZE
  uint32_t length;

// 3.1.3 SOURCE
  std::string source;

// 3.1.4 TYPE :="DCSH"
  std::string type;

// 3.1.4.1 EXP_FILL
  std::string expansion;

// 3.1.4.2 HDR_CRC32
  uint32_t headerCRC;
  uint32_t fileCRC;

  int readFrom(const char* buf, size_t len);
};

struct DCPData {
  // Keep information in the provided spec format as much as possible.  Formatting will be done in formatData for output

  struct blockData {

// 3.2.1
    uint8_t blockID;

//3.2.2
    uint16_t blockLength;

// 3.3.1 Table
    uint32_t sequence;

// 3.3.1.1 Message Flags/Baud
// Parity errors used to define 0-ASCII, 1-Pseudo-Binary
    uint16_t baudRate;
    uint8_t platform;
    bool parityErrors;
    bool missingEOT;
    bool msgflagb6;
    bool msgflagb7;

// Parse arm 3.3.1.2
    bool addrCorrected;
    bool badAddr;
    bool invalidAddr;
    bool incompletePDT;
    bool timingError;
    bool unexpectedMessage;
    bool wrongChannel;
    bool armflagb7;

// 3.3.1.3
    uint32_t correctedAddr;
    
// 3.3.1.4
    std::string carrierStart;

// 3.3.1.5
    std::string carrierEnd;

// 3.3.1.6

    float signalStrength;

// 3.3.1.7
    float freqOffset;

// 3.3.1.8
    float phaseNoise;
    std::string phaseModQuality;

// 3.3.1.9 
    float goodPhase;

// 3.3.1.10 Channel
    std::string spacePlatform;
    uint16_t channelNumber;

// 3.3.1.11 Source Code
    std::string sourcePlatform;

// 3.3.1.2 Source Secondary
    std::vector<uint8_t> sourceSecondary = { 0, 0 }; // Hack.  Change when this field is used by system. The data type is TBD ..Still not used in April 2022 twc

// 3.3.2
    std::vector<uint8_t> DCPData;

// 3.2.2
    uint16_t DCPDataLength;

// 3.2.4
    uint16_t blockCRC;
  };

  std::unordered_map<std::string, std::string> spMap;
  std::vector<blockData> block_data;
  int readFrom(const char* buf, size_t len);
};

void initMap(std::unordered_map<std::string, std::string> &data);
std::string toPhaseModQuality(const uint16_t& data);
std::string toSpacePlatform(const uint16_t& data);
uint16_t toRate(const uint8_t& data);
std::string toDateTime(const std::vector<uint8_t>& data);
std::vector<char> formatData(const FileHeader &fh, const DCPData &dcp);

} // namespace dcs
