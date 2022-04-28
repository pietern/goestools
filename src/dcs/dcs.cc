#include "dcs.h"
#include <cstring>
#include <iostream>
#include <array>
#include <regex>
//#include <boost/crc.hpp>
//#include <boost/integer.hpp>

#include "util/error.h"
#include "sutron.h"

namespace dcs {

int FileHeader::readFrom(const char* buf, size_t len) {
	size_t nread = 0;

  // The DCS file header is 64 bytes

  // 32 bytes contain the DCS file name (and trailing spaces)
  {
    constexpr unsigned n = 32;
    ASSERT((len - nread) >= n);
    name = std::string(buf + nread, n);
    nread += n;
  }

  // 8 bytes contain length of payload
  {
    constexpr unsigned n = 8;
    ASSERT((len - nread) >= n);
    length = std::stoi(std::string(buf + nread, n));
    ASSERT(len == length); // Payload reported length does not equal what we actually received
    nread += n;
  }

  // 4 bytes contain source of payload
  // Jan 7, 2021: 
  //	  Data sourced from Wallops Command and Data Acquisition station will use the code “WCDA”
  //  Data sourced from NOAA’s Satellite Operations Facility will use the code “NSOF”
  {
    constexpr unsigned n = 4;
    ASSERT((len - nread) >= n);
    source = std::string(buf + nread, n);
    nread += n;
  }

  // 4 bytes contain type of payload
  {
	  constexpr unsigned n = 4;
	  ASSERT((len - nread) >= n);
	  type = std::string(buf + nread, n);
	  nread += n;
  }

  // 12 bytes for future use
  {
	  constexpr unsigned n = 12;
	  ASSERT((len - nread) >= n);
	  expansion = std::string(buf + nread, n);
	  nread += n;
  }

  // 4 bytes contain header CRC32
  {
	  constexpr unsigned n = 4;
	  ASSERT((len - nread) >= n);
	  memcpy(&headerCRC, &buf[nread], n);
	  nread += n;
  }

  // 4 bytes contain end file CRC32
  {
    constexpr unsigned n = 4;
    ASSERT((len - nread) >= n);
    memcpy(&fileCRC, &buf[length - n], n); // CRC is at the end of the DCS payload
    nread += n;
  }
  return nread;
}

int DCPData::readFrom(const char* buf, size_t len) {
  size_t nread = 64; // Start position of message block_data after file header
  uint16_t pos = 0;


  while (nread < len - 4) { // 4 for ending header CRC32 per spec
    block_data.resize(pos + 1);

    // 1 byte contains block ID
    {
      constexpr unsigned n = 1;
      ASSERT((len - nread) >= n);
      memcpy(&block_data[pos].blockID, &buf[nread], n);
      nread += n;
    }

    // Skip over Missed Messages Blocks (blockID == 2 as of January 21, 2021) 
    if (block_data[pos].blockID != 1) {
      nread += 28; // Size of Missed Message Block (2(block len) + 24(header) + 2(CRC16))
    }
    else {

      // 2 bytes contain block length
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        memcpy(&block_data[pos].blockLength, &buf[nread], n);
        nread += n;
      }

      // Begin DCP Header section

      // 3 bytes contain sequence number
      {
        constexpr unsigned n = 3;
        ASSERT((len - nread) >= n);
        std::vector<uint8_t> tmp;
        tmp.resize(n);
        memcpy(tmp.data(), &buf[nread], n);
        block_data[pos].sequence = tmp.at(0) | (tmp.at(1) << 8) | (tmp.at(2) << 16);

        nread += n;
      }

      // 1 byte contains message flags
      {
        constexpr unsigned n = 1;
        ASSERT((len - nread) >= n);
        uint8_t tmp;
        memcpy(&tmp, &buf[nread], n);
        block_data[pos].baudRate = toRate(tmp);
        block_data[pos].platform = tmp & 0x8 >> 3;
        block_data[pos].parityErrors = tmp & 0x10;
        block_data[pos].missingEOT = tmp & 0x20;
//        block_data[pos].msgflagb6 = tmp & 0x40;
//        block_data[pos].msgflagb7 = tmp & 0x7F;

        nread += n;
      }

      // 1 byte contains abnormal message flags
      {
        constexpr unsigned n = 1;
        ASSERT((len - nread) >= n);
        uint8_t tmp;
        memcpy(&tmp, &buf[nread], n);
        block_data[pos].addrCorrected = tmp & 0x1;
        block_data[pos].badAddr = tmp & 0x2;
        block_data[pos].invalidAddr = tmp & 0x4;
        block_data[pos].incompletePDT = tmp & 0x8;
        block_data[pos].timingError = tmp & 0x10;
        block_data[pos].unexpectedMessage = tmp & 0x20;
        block_data[pos].wrongChannel = tmp & 0x40;
//        block_data[pos].armflagb7 = tmp & 0x7F;
        nread += n;
      }

      // 4 bytes contain received platform (i.e. "ground station") address
      {
        constexpr unsigned n = 4;
        ASSERT((len - nread) >= n);
        memcpy(&block_data[pos].correctedAddr, &buf[nread], n);
        nread += n;
      }

      // 7 bytes contain carrier start time in BCD format.  Time when signal energy received by space platform began.
      {
        constexpr unsigned n = 7;
        ASSERT((len - nread) >= n);
        std::vector<uint8_t> tmp;
        tmp.resize(n);
        memcpy(tmp.data(), &buf[nread], n);
        block_data[pos].carrierStart = toDateTime(tmp);
        nread += n;
      }

      // 7 bytes contain carrier end time in BCD format.  Time when signal energy received by space platform stopped.
      {
        constexpr unsigned n = 7;
        ASSERT((len - nread) >= n);
        std::vector<uint8_t> tmp;
        tmp.resize(n);
        memcpy(tmp.data(), &buf[nread], n);
        block_data[pos].carrierEnd = toDateTime(tmp);
        nread += n;
      }

      // 2 bytes contain signal strength received at space platform (dBm EIRP)
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        block_data[pos].signalStrength = (float)(0x3ff & tmp) / 10.0; // Keep 10 LSBits and divide by 10 per data spec
        nread += n;
      }

      // 2 bytes contain received frequency offset from the expected carrier frequency
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        block_data[pos].freqOffset = (float)(int16_t)((tmp & 0x3fff) | (((tmp & 0x2000) << 2) | ((tmp & 0x2000) << 1))) / 10.0; // Keep 14 LSBits, add sign, and divide by 10 per data spec
        nread += n;
      }

      // 2 bytes contain received signal phase noise (degrees) and modulation index
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        block_data[pos].phaseNoise = (float)(tmp & 0xfff) / 100.0; // Keep 12 LSBits and divide by 100 per data spec
        block_data[pos].phaseModQuality = toPhaseModQuality(tmp); // Keep 2 MSBits per data spec (00 = Unknown, 01 = Normal, 10 = High, 11 = Low (as of Jan 9, 2021))
        nread += n;
      }

      // 1 byte contains a representation of the received signal quality (percentage) by the platform ('>=85% = GOOD', '70%<= <85% = FAIR', '<70% = POOR').  
      {
        constexpr unsigned n = 1;
        ASSERT((len - nread) >= n);
        uint8_t tmp;
        memcpy(&tmp, &buf[nread], n);
        block_data[pos].goodPhase = tmp / 2.0;
        nread += n;
      }

      // 2 bytes contain the space platform and channel number
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        uint16_t tmp;
        memcpy(&tmp, &buf[nread], n);
        block_data[pos].spacePlatform = toSpacePlatform(tmp);
        block_data[pos].channelNumber = tmp & 0xFFF;
        nread += n;
      }

      // 2 bytes contain the source platform the message was received on.  This is the not the client platform but instead the main "hub" platform.
      // See spec document for a list of platforms.
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        std::string tmp;
        tmp = std::string(buf + nread, n);
//        block_data[pos].sourcePlatform = spMap[tmp];
        block_data[pos].sourcePlatform = tmp;
        nread += n;
      }

      // 2 bytes contain the secondary source (not used as of April 12, 2022)
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        block_data[pos].sourceSecondary.resize(n);
        memcpy(block_data[pos].sourceSecondary.data(), &buf[nread], n);
        nread += n;
      }
      // End DCP Header section

      // Variables bytes contain the message data from the client ("ground station")
      {
        // Message data size.  Total size subtract [DCP Header size (36(DCP Header) + 1(Block ID) + 2(Block Length) + 2(CRC16))]
        uint16_t n = block_data[pos].blockLength - 41;
        ASSERT((len - nread) >= n);
        block_data[pos].DCPData.resize(n);
        memcpy(block_data[pos].DCPData.data(), &buf[nread], n);
        block_data[pos].DCPDataLength = n;
        nread += n;
      }

      // 2 bytes contain block CRC16
      {
        constexpr unsigned n = 2;
        ASSERT((len - nread) >= n);
        memcpy(&block_data[pos].blockCRC, &buf[nread], n);
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
    std::string tmp = "File Header CRC: " + std::to_string(fh.headerCRC);
    std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    buf.push_back('\n');
  }

  uint16_t pos = 0;

  while (pos < dcp.block_data.size()) {

    {
      std::string tmp = "\n----------[ DCP Block ]----------\nHeader:\n\tBlock ID: " + std::to_string(dcp.block_data[pos].blockID);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }


    {
      std::string tmp = "\tBlock Length: " + std::to_string(dcp.block_data[pos].blockLength);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tSequence: " + std::to_string(dcp.block_data[pos].sequence);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tFlags:\n\t\tData Rate: " + std::to_string(dcp.block_data[pos].baudRate);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\t\tPlatform: " + std::string(dcp.block_data[pos].platform ? "CS1" : "CS2");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\t\tParity Errors: " + std::string(dcp.block_data[pos].parityErrors ? "Yes" : "No");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

/*
    {
      std::string tmp = " msgflagb6: " + std::string(dcp.block_data[pos].msgflagb6 ? "Yes" : "No");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    } 

    {
      std::string tmp = " msgflagb7: " + std::string(dcp.block_data[pos].msgflagb7 ? "Yes" : "No");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    } 
  
    {
      std::string tmp = " armflagb7: " + std::string(dcp.block_data[pos].armflagb7 ? "Yes" : "No");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    } 
*/
    {
      std::string tmp = "\tARM Flag: ";
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.block_data[pos].addrCorrected ? "A" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp =  std::string(dcp.block_data[pos].badAddr ? "B" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.block_data[pos].invalidAddr ? "I" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.block_data[pos].incompletePDT ? "N" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.block_data[pos].timingError ? "T" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.block_data[pos].unexpectedMessage ? "U" : "");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      std::string tmp = std::string(dcp.block_data[pos].wrongChannel ? "W" : "G");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
    }

    {
      buf.push_back('\n');
      std::stringstream addr;
      addr << std::hex << dcp.block_data[pos].correctedAddr;
      std::string tmp = "\tCorrected Address: " + addr.str();
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tCarrier Start (UTC): ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.block_data[pos].carrierStart.begin(), dcp.block_data[pos].carrierStart.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tCarrier End (UTC): ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.block_data[pos].carrierEnd.begin(), dcp.block_data[pos].carrierEnd.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tSignal Strength(dBm EIRP): " + std::to_string(dcp.block_data[pos].signalStrength);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tFrequency Offset(Hz): " + std::to_string(dcp.block_data[pos].freqOffset);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tPhase Noise(Degrees RMS): " + std::to_string(dcp.block_data[pos].phaseNoise);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tModulation Index: ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.block_data[pos].phaseModQuality.begin(), dcp.block_data[pos].phaseModQuality.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tGood Phase (%): " + std::to_string(dcp.block_data[pos].goodPhase);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp = "\tChannel: " + std::to_string(dcp.block_data[pos].channelNumber);
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tSpaceCraft: ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.block_data[pos].spacePlatform.begin(), dcp.block_data[pos].spacePlatform.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      std::string tmp("\tSource Code: ");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::copy(dcp.block_data[pos].sourcePlatform.begin(), dcp.block_data[pos].sourcePlatform.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
      // Currently not used.  Just save what is there not knowing the format.  Note that sourceSecondary is initialized to a length of 2 in declaration.
      std::string tmp = "\tSource Secondary: " + std::to_string(dcp.block_data[pos].sourceSecondary.at(0)) + std::to_string(dcp.block_data[pos].sourceSecondary.at(1));
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      buf.push_back('\n');
    }

    {
/*    ---------- Taking out the Boost CRC stuff, haven't figured it out ------
	//  Test CRC-16  below web
	// https://www.lammertbies.nl/comm/info/crc-calculation
	//unsigned char const data[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	std::size_t data_len =  dcp.block_data[pos].blockLength;
	boost::uint16_t const expected_result = dcp.block_data[pos].blockCRC;
	boost::crc_ccitt_type checksum_agent;
	std::string tmp="Block CRC: OK";
*/

	uint16_t const expected_result = dcp.block_data[pos].blockCRC;
	std::string tmp="Block CRC: " + std::to_string(expected_result);
	std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
	buf.push_back('\n');

/*    ---------- Taking out the Boost stuff for now ------
  	data_len=data_len-2; // all but the last 2 which are added CRC when sent
	checksum_agent.process_bytes(reinterpret_cast<const char *>(&dcp.block_data[pos]), data_len);

	if ( checksum_agent.checksum() != expected_result ) 
	{
       		tmp = "Block CRC: FAIL " +  std::to_string(checksum_agent.checksum());
     		std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
	        buf.push_back('\n');
	}
*/

    }

    {
      // To output decoded ascii from  Compaction, 6-bit packed ascii ("Pseudo-Binary")
      std::string tmp("Data (Pseudo-Binary): \n");
      std::copy(tmp.begin(), tmp.end(), std::back_inserter(buf));
      std::string ascii, tmp2;
      char unpacked[5]={0};
	for (unsigned j = 0; j < dcp.block_data[pos].DCPDataLength; j++)
	{	
	// Read through 6-bit packed data
	  for(int i=0;i<4;++i)
	  {
		unpacked[i]=dcp.block_data[pos].DCPData[j]>>(6*(3-i)) & 0x3F;   // Divide into block_data of 6
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
