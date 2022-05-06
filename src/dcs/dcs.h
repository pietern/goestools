// From  https://dcs1.noaa.gov/documents/HRIT%20DCS%20File%20Format%20Rev1.pdf
#pragma once

#include <unistd.h>

#include <ctime>
#include <string>
#include <vector>
#include <unordered_map>

namespace dcs {

// Header at the beginning of an LRIT DCS file
 
// 3.1 File Header 64 bytes
struct FileHeader {

// 3.1.1 FILE_NAME 32 bytes
  std::string name;

// 3.1.2 FILE_SIZE:= 8 bytes
  uint32_t length;

// 3.1.3 SOURCE 4 bytes
  std::string source;

// 3.1.4 TYPE :="DCSH" 4 bytes
  std::string type;

// 3.1.4.1 EXP_FILL 12 bytes
  std::string expansion;

// 3.1.4.2 HDR_CRC32 4 bytes
  uint32_t headerCRC;

// 3.5 FILE_CRC32:= 4 Bytes
  uint32_t fileCRC;

  int readFrom(const char* buf, size_t len);
};

struct DCPData {
  // Keep information in the provided spec format as much as possible.  Formatting will be done in formatData for output

  struct blockData {

// 3.2.1 BLK_ID 1 byte
    uint8_t blockID;

//3.2.2 BLK_LNG 2 bytes
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

// 3.3.1.2 Message ARM Flag
    bool addrCorrected;
    bool badAddr;
    bool invalidAddr;
    bool incompletePDT;
    bool timingError;
    bool unexpectedMessage;
    bool wrongChannel;
    bool armflagb7;

// 3.3.1.3 Corrected Address
    uint32_t correctedAddr;
    
// 3.3.1.4 Carrier Start
    std::string carrierStart;

// 3.3.1.5 Carrier End
    std::string carrierEnd;

// 3.3.1.6 Signal Strength X10

    float signalStrength;

// 3.3.1.7 Frequency Offset X10
    float freqOffset;

// 3.3.1.8 Phase Noise X100 and Modulation Index
    float phaseNoise;
    std::string phaseModQuality;

// 3.3.1.9  Good Phase X2
    float goodPhase;

// 3.3.1.10 Channel/Spacecraft
    std::string spacePlatform;
    uint16_t channelNumber;

// 3.3.1.11 Source Code= 2 bytes
    std::string sourcePlatform;

// 3.3.1.2 Source Secondary:= 2 bytes
    std::string sourceSecondary;

// 3.3.2 DCP_DATA:=0..
    std::vector<uint8_t> DCPData;

// 3.2.2 BLK_LNG:=2 bytes
    uint16_t DCPDataLength;

// 3.2.3 BLK_DATA:=0..65,530 bytes

// 3.2.4 BLK_CRC16:=2 bytes
    uint16_t blockCRC;
  };

  std::unordered_map<std::string, std::string> spMap;
  std::vector<blockData> blocks;
  int readFrom(const char* buf, size_t len);
};

void initMap(std::unordered_map<std::string, std::string> &data);
std::string toPhaseModQuality(const uint16_t& data);
std::string toSpacePlatform(const uint16_t& data);
uint16_t toRate(const uint8_t& data);
std::string toDateTime(const std::vector<uint8_t>& data);
std::vector<char> formatData(const FileHeader &fh, const DCPData &dcp);

} // namespace dcs
