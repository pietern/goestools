#include "dcs.h"
#include <cstring>
#include <iostream>
#include <array>
#include <regex>

#include "util/error.h"
#include "sutron.h"

namespace dcs {

int FileHeader::readFrom(const char* buf, size_t len) {
	size_t nread = 0;

  // The DCS file header is 64 bytes

  // 3.1.1 FILE_NAME:=32 bytes
  {
	constexpr unsigned n = 32;
	ASSERT((len - nread) >= n);
	name = std::string(buf + nread, n);
	nread += n;
  }

  // 3.1.2 FIL_SIZE:= 8 bytes
  {
	constexpr unsigned n = 8;
	ASSERT((len - nread) >= n);
	length = std::stoi(std::string(buf + nread, n));
	ASSERT(len == length); // Payload reported length does not equal what we actually received
	nread += n;
  }

  // 3.3.3 SOURCE:= 4 bytes
  {
	constexpr unsigned n = 4;
	ASSERT((len - nread) >= n);
	source = std::string(buf + nread, n);
	nread += n;
  }

  // 3.1.4 TYPE:= 4 bytes ("DCSH")
  {
	  constexpr unsigned n = 4;
	  ASSERT((len - nread) >= n);
	  type = std::string(buf + nread, n);
	  nread += n;
  }

  // 3.1.4.1 EXP_FILL:= 12 bytes (EXP_FLD, EXP_FIELD)
  {
	  constexpr unsigned n = 12;
	  ASSERT((len - nread) >= n);
	  expansion = std::string(buf + nread, n);
	  nread += n;
  }

  // 3.1.4.2 HDR_CRC32:= 4 bytes
  {
	  constexpr unsigned n = 4;
	  ASSERT((len - nread) >= n);
	  memcpy(&headerCRC, &buf[nread], n);
	  nread += n;
  }

  // 3.5 FILE_CRC32:= 4 bytes
  {
	constexpr unsigned n = 4;
	ASSERT((len - nread) >= n);
	memcpy(&fileCRC, &buf[length - n], n); // CRC is at the end of the DCS payload
	nread += n;
  }

	
  return nread;
}

int DCPData::readFrom(const char* buf, size_t len) {
  size_t nread = 64; // Start position of message block after file header
  uint16_t pos = 0;


  while (nread < len - 4) { // 4 for ending header CRC32 per spec
    blocks.resize(pos + 1);

    // 1 byte contains blocks ID
    {
      constexpr unsigned n = 1;
      ASSERT((len - nread) >= n);
      memcpy(&blocks[pos].blockID, &buf[nread], n);
      nread += n;
    }

         // 2 bytes contain blocks length
	{
         constexpr unsigned n = 2;
         ASSERT((len - nread) >= n);
         memcpy(&blocks[pos].blockLength, &buf[nread], n);
         nread += n;
	}
     if (blocks[pos].blockID != 1)
     {
    //  Skip over Missed Messages Blocks (blockID == 2 per HRITS Doc 1/25/2019)
	nread += blocks[pos].blockLength - 3; //Skip(header + 2(CRC16))-(ID+LNG)
       // nread += 26; // Size of Missed Message Block (24(header) + 2(CRC16))
    }
    else 
    {
      // Begin DCP Header section

      // 3 bytes contain sequence number
      {
        constexpr unsigned n = 3;
        ASSERT((len - nread) >= n);
        std::vector<uint8_t> tmp;
        tmp.resize(n);
        memcpy(tmp.data(), &buf[nread], n);
        blocks[pos].sequence = tmp.at(0) | (tmp.at(1) << 8) | (tmp.at(2) << 16);

        nread += n;
      }

      // 1 byte contains message flags
      {
        constexpr unsigned n = 1;
        ASSERT((len - nread) >= n);
        uint8_t tmp;
        memcpy(&tmp, &buf[nread], n);
        blocks[pos].baudRate = toRate(tmp);
        blocks[pos].platform = tmp & 0x8 >> 3;
        blocks[pos].parityErrors = tmp & 0x10;
        blocks[pos].missingEOT = tmp & 0x20;

        nread += n;
      }

      // 1 byte contains abnormal message flags
      {
        constexpr unsigned n = 1;
        ASSERT((len - nread) >= n);
        uint8_t tmp;
        memcpy(&tmp, &buf[nread], n);
        blocks[pos].addrCorrected = tmp & 0x1;
        blocks[pos].badAddr = tmp & 0x2;
        blocks[pos].invalidAddr = tmp & 0x4;
        blocks[pos].incompletePDT = tmp & 0x8;
        blocks[pos].timingError = tmp & 0x10;
        blocks[pos].unexpectedMessage = tmp & 0x20;
        blocks[pos].wrongChannel = tmp & 0x40;
        nread += n;
      }

      // 4 bytes contain received platform (i.e. "ground station") address
      {
        constexpr unsigned n = 4;
        ASSERT((len - nread) >= n);
        memcpy(&blocks[pos].correctedAddr, &buf[nread], n);
        nread += n;
      }

      // 7 bytes contain carrier start time in BCD format.  Time when signal energy received by space platform began.
      {
        constexpr unsigned n = 7;
        ASSERT((len - nread) >= n);
        std::vector<uint8_t> tmp;
        tmp.resize(n);
        memcpy(tmp.data(), &buf[nread], n);
        blocks[pos].carrierStart = toDateTime(tmp);
        nread += n;
      }

      // 7 bytes contain carrier end time in BCD format.  Time when signal energy received by space platform stopped.
      {
        constexpr unsigned n = 7;
        ASSERT((len - nread) >= n);
        std::vector<uint8_t> tmp;
        tmp.resize(n);
        memcpy(tmp.data(), &buf[nread], n);
        blocks[pos].carrierEnd = toDateTime(tmp);
        nread += n;
      }

      // 2 bytes contain signal strength received at space platform (dBm EIRP)
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        blocks[pos].signalStrength = (float)(0x3ff & tmp) / 10.0; // Keep 10 LSBits and divide by 10 per data spec
        nread += n;
      }

      // 2 bytes contain received frequency offset from the expected carrier frequency
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        blocks[pos].freqOffset = (float)(int16_t)((tmp & 0x3fff) | (((tmp & 0x2000) << 2) | ((tmp & 0x2000) << 1))) / 10.0; // Keep 14 LSBits, add sign, and divide by 10 per data spec
        nread += n;
      }

      // 2 bytes contain received signal phase noise (degrees) and modulation index
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        blocks[pos].phaseNoise = (float)(tmp & 0xfff) / 100.0; // Keep 12 LSBits and divide by 100 per data spec
        blocks[pos].phaseModQuality = toPhaseModQuality(tmp); // Keep 2 MSBits per data spec (00 = Unknown, 01 = Normal, 10 = High, 11 = Low (as of Jan 9, 2021))
        nread += n;
      }

      // 1 byte contains a representation of the received signal quality (percentage) by the platform ('>=85% = GOOD', '70%<= <85% = FAIR', '<70% = POOR').  
      {
        constexpr unsigned n = 1;
        ASSERT((len - nread) >= n);
        uint8_t tmp;
        memcpy(&tmp, &buf[nread], n);
        blocks[pos].goodPhase = tmp / 2.0;
        nread += n;
      }

      // 2 bytes contain the space platform and channel number
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        blocks[pos].spacePlatform = toSpacePlatform(tmp);
        blocks[pos].channelNumber = tmp & 0xFFF;
        nread += n;
      }

      // 2 bytes contain the source platform the message was received on.  This is the not the client platform but instead the main "hub" platform.
      // See spec document for a list of platforms.
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        std::string tmp;
        tmp = std::string(buf + nread, n);
        blocks[pos].sourcePlatform = tmp;
        nread += n;
      }

      // 2 bytes contain the secondary source (not used as of April 12, 2022)
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        std::string tmp;
	tmp=""; // hack as there is nothing sent yet
        tmp = std::string(buf + nread, n);
        blocks[pos].sourceSecondary = tmp;
        nread += n;
      }
      // End DCP Header section

      // Variables bytes contain the message data from the client ("ground station")
      {
        // Message data size.  Total size subtract [DCP Header size (36(DCP Header) + 1(Block ID) + 2(Block Length) + 2(CRC16))]
        uint16_t n = blocks[pos].blockLength - 41;
        ASSERT((len - nread) >= n);
        blocks[pos].DCPData.resize(n);
        memcpy(blocks[pos].DCPData.data(), &buf[nread], n);
        blocks[pos].DCPDataLength = n;
        nread += n;
      }

      // 2 bytes contain blocks CRC16
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        memcpy(&blocks[pos].blockCRC, &buf[nread], n);
        nread += n;
      }
    }
    pos++;
  }
  return nread;
}

std::string toPhaseModQuality(const uint16_t& data) {
  switch ((data & 0xc000) >> 14) {
  case 0: return "Unknown";
  case 1: return "Normal";
  case 2: return "High";
  case 3: return "Low";
  default: return "";
  }
}
// Table 5
std::string toSpacePlatform(const uint16_t& data) {
  switch ((data & 0xF000) >> 12) {
  case 0: return "Unknown";
  case 1: return "GOES-East";
  case 2: return "GOES-West";
  case 3: return "GOES-Central";
  default: return "";
  }
}

uint16_t toRate(const uint8_t& data) {
  switch (data & 0x7) {
  case 1: return 100;
  case 2: return 300;
  case 3: return 1200;
  default: return 0;
  }
}

std::string toDateTime(const std::vector<uint8_t>& data) {

  ASSERT(data.size() == 7); // Expect 7 bytes

  // YYDDDHHMMSSZZZ – The day is represented as a three digit day of the year (julian day)
  auto year = (((data.at(6) & 0xF0) >> 4) * 10) + (data.at(6) & 0x0F); // Nibble math
  auto day = (((data.at(5) & 0xF0) >> 4) * 100) + ((data.at(5) & 0x0F) * 10) + ((data.at(4) & 0xF0) >> 4);
  auto hour = ((data.at(4) & 0x0F) * 10) + (((data.at(3) & 0xF0) >> 4));
  auto minute = ((data.at(3) & 0x0F) * 10) + ((data.at(2) & 0xF0) >> 4);
  auto second = ((data.at(2) & 0x0F) * 10) + ((data.at(1) & 0xF0) >> 4);
  auto subSecond = ((data.at(1) & 0x0F) * 100) + (((data.at(0) & 0xF0) >> 4) * 10) + (data.at(0) & 0x0F);

  struct tm tm;
  struct timespec time;
  memset(&tm, 0, sizeof(tm)); // Set values to zero
  memset(&time, 0, sizeof(time));

  // The number of years since 1900.
  tm.tm_year = 100 + year;

  // Use offset at beginning of year and add parts
  time.tv_sec = mktime(&tm) + ((((day * 24) + hour) * 60) + minute) * 60 + second;
  time.tv_nsec = subSecond * 1000000;

 std::array<char, 128> buf;
  memset(&buf, 0, sizeof(buf));
  auto len = strftime(buf.data(), buf.size(), "%c", gmtime(&time.tv_sec));

  return std::string(buf.data(), len);
}

  std::vector<char> formatData(const dcs::FileHeader &fh, const dcs::DCPData &dcp) {
  std::vector<char> buf;

  {
    std::string tmp("----------[ DCS Header Info ]----------\nFilename: ");
    std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    std::copy(fh.name.begin(), fh.name.end(), std::back_inserter(buf));
    buf.push_back('\n');
  }

  {
    std::string tmp = "File Size: " + std::to_string(fh.length);
    std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    buf.push_back('\n');
  }

  {
    std::string tmp("File Source: ");
    std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    std::copy(fh.source.begin(), fh.source.end(), std::back_inserter(buf));
    buf.push_back('\n');
  }

  {
    std::string tmp("File Type: ");
    std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    std::copy(fh.type.begin(), fh.type.end(), std::back_inserter(buf));
    buf.push_back('\n');
  }


  {
	{
		std::string tmp="HDR_CRC32: " + to_string(fh.headerCRC);
		std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
		buf.push_back('\n');
	}
	{
		std::string tmp="FILE_CRC32: " + to_string(fh.fileCRC);
		std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
		buf.push_back('\n');
	}
  }

  uint16_t pos = 0;

  while (pos < dcp.blocks.size()) {

    {
      std::string tmp = "\n----------[ DCP Block ]----------\nHeader:\n\tBlock ID: " + std::to_string(dcp.blocks[pos].blockID);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }


    {
      std::string tmp = "\tBlock Length: " + std::to_string(dcp.blocks[pos].blockLength);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tSequence: " + std::to_string(dcp.blocks[pos].sequence);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tFlags:\n\t\tData Rate: " + std::to_string(dcp.blocks[pos].baudRate);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\t\tPlatform: " + std::string(dcp.blocks[pos].platform ? "CS1" : "CS2");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\t\tParity Errors: " + std::string(dcp.blocks[pos].parityErrors ? "Yes" : "No");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tARM Flag: ";
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.blocks[pos].addrCorrected ? "A" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp =  std::string(dcp.blocks[pos].badAddr ? "B" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.blocks[pos].invalidAddr ? "I" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.blocks[pos].incompletePDT ? "N" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.blocks[pos].timingError ? "T" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.blocks[pos].unexpectedMessage ? "U" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.blocks[pos].wrongChannel ? "W" : "G");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      buf.push_back('\n');
      std::stringstream addr;
      addr << std::hex << dcp.blocks[pos].correctedAddr;
      std::string tmp = "\tCorrected Address: " + addr.str();
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tCarrier Start (UTC): ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.blocks[pos].carrierStart.begin(), dcp.blocks[pos].carrierStart.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tCarrier End (UTC): ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.blocks[pos].carrierEnd.begin(), dcp.blocks[pos].carrierEnd.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tSignal Strength(dBm EIRP): " + std::to_string(dcp.blocks[pos].signalStrength);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tFrequency Offset(Hz): " + std::to_string(dcp.blocks[pos].freqOffset);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tPhase Noise(Degrees RMS): " + std::to_string(dcp.blocks[pos].phaseNoise);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tModulation Index: ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.blocks[pos].phaseModQuality.begin(), dcp.blocks[pos].phaseModQuality.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tGood Phase (%): " + std::to_string(dcp.blocks[pos].goodPhase);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tChannel: " + std::to_string(dcp.blocks[pos].channelNumber);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tSpaceCraft: ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.blocks[pos].spacePlatform.begin(), dcp.blocks[pos].spacePlatform.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tSource Code: ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.blocks[pos].sourcePlatform.begin(), dcp.blocks[pos].sourcePlatform.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp =( "\tSource Secondary: ");
      std::copy(dcp.blocks[pos].sourceSecondary.begin(), dcp.blocks[pos].sourceSecondary.end(), std::back_inserter(buf));
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {

		std::string tmp="BLOCK_CRC16: " + to_string(dcp.blocks[pos].blockCRC);
		std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
		buf.push_back('\n');

    }

    {
      // To output decoded ascii from  Compaction, 6-bit packed ascii ("Pseudo-Binary")
      std::string tmp("Data (Pseudo-Binary): \n");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::string ascii, tmp2;
      char unpacked[5]={0};
	for (unsigned j = 0; j < dcp.blocks[pos].DCPDataLength; j++)
	{	
	// Read through 6-bit packed data
	  for(int i=0;i<4;++i)
	  {
		unpacked[i]=dcp.blocks[pos].DCPData[j]>>(6*(3-i)) & 0x3F;   // Divide into blocks of 6
		//Decode Compacted Pseudo Binary (25 % word size reduction). Compaction drops bits 6 & 7
		// Adding back bit 6 "0x40" where needed.  ie any letter (bit 5 is 0)
		if ( unpacked[i] < 0x20)
			ascii=unpacked[i]+0x40;
		else
			ascii=unpacked[i];
	  }

		std::copy(ascii.begin(), ascii.end(), std::back_inserter(tmp2));
	}
	// From Appendix B DCS_CRS_final_June09.pdf.  All data is Pseudo-Binary
	// encoded.
	// The first byte contains the format types of data.
	// This word is used to decode the following data type structure.
	// This is a possible 0-63, 64 different format types. 
	// Some are readable by humans. See https://github.com/opendcs for
	//  more info.   
	//
	// Common types: Ascii, SHEF, Sutron Types: B,C,D .. 64 of them.
	   tmp2 = sutron(tmp2); // convert the known Sutron formats, to human 
           std::copy(tmp2.begin(), tmp2.end(), std::back_inserter(buf));
        buf.push_back('\n');
     }




    buf.push_back('\n');
    pos++;
  }

  buf.push_back('\n');
  return buf;
}

} // namespace dcs
