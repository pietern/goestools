#include "dcs.h"

#include <cstring>
#include <iostream>

#include <util/error.h>

namespace dcs {

int FileHeader::readFrom(const char* buf, size_t len) {
  size_t nread = 0;

  // 32 bytes hold the DCS file name (and trailing spaces)
  {
    constexpr unsigned n = 32;
    ASSERT((len - nread) >= n);
    name = std::string(buf + nread, n);
    nread += n;
  }

  // 8 bytes holding length of payload
  {
    constexpr unsigned n = 8;
    ASSERT((len - nread) >= n);
    length = std::stoi(std::string(buf + nread, n));
    nread += n;
  }

  // 20 bytes holding some ASCII data
  {
    constexpr unsigned n = 20;
    ASSERT((len - nread) >= n);
    misc1 = std::string(buf + nread, n);
    nread += n;
  }

  // 8 bytes holding some binary data
  {
    constexpr unsigned n = 8;
    ASSERT((len - nread) >= n);
    misc2.resize(n);
    memcpy(misc2.data(), &buf[nread], n);
    nread += n;
  }

  return nread;
}

int Header::readFrom(const char* buf, size_t len) {
  size_t nread = 0;

  // 8 hex digit DCP Address
  {
    constexpr unsigned n = 8;
    ASSERT((len - nread) >= 8);
    auto rv = sscanf(buf + nread, "%08lx", &address);
    ASSERT(rv == 1);
    nread += n;
  }

  // YYDDDHHMMSS – Time the message arrived at the Wallops receive station.
  // The day is represented as a three digit day of the year (julian day).
  {
    constexpr unsigned n = 11;
    ASSERT((len - nread) >= n);
    auto year = std::stoi(std::string(buf + nread, 2), nullptr, 10);
    nread += 2;
    auto day = std::stoi(std::string(buf + nread, 3), nullptr, 10);
    nread += 3;
    auto hour = std::stoi(std::string(buf + nread, 2), nullptr, 10);
    nread += 2;
    auto minute = std::stoi(std::string(buf + nread, 2), nullptr, 10);
    nread += 2;
    auto second = std::stoi(std::string(buf + nread, 2), nullptr, 10);
    nread += 2;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    // The number of years since 1900.
    tm.tm_year = 100 + year;

    // Use offset at beginning of year and add parts
    time.tv_sec = mktime(&tm) + ((((day * 24) + hour) * 60) + minute) * 60 + second;
    time.tv_nsec = 0;
  }

  // 1 character failure code
  {
    constexpr unsigned n = 1;
    ASSERT((len - nread) >= n);
    failure = buf[nread];
    nread += n;
  }

  // 2 decimal digit signal strength
  {
    constexpr unsigned n = 2;
    ASSERT((len - nread) >= n);
    signalStrength = std::stoi(std::string(buf + nread, 2));
    nread += n;
  }

  // 2 decimal digit frequency offset
  {
    constexpr unsigned n = 2;
    ASSERT((len - nread) >= n);
    // This can be +A or -A; don't know what it means...
    if ((buf[nread] == '+' || buf[nread] == '-') && buf[nread+1] == 'A') {
      frequencyOffset = 0;
    } else {
      frequencyOffset = std::stoi(std::string(buf + nread, n));
    }
    nread += n;
  }

  // 1 character modulation index
  {
    constexpr unsigned n = 1;
    ASSERT((len - nread) >= n);
    modulationIndex = buf[nread];
    nread += n;
  }

  // 1 character data quality indicator
  {
    constexpr unsigned n = 1;
    ASSERT((len - nread) >= n);
    dataQuality = buf[nread];
    nread += n;
  }

  // 3 decimal digit GOES receive channel
  {
    constexpr unsigned n = 3;
    ASSERT((len - nread) >= n);
    receiveChannel = std::stoi(std::string(buf + nread, n));
    nread += n;
  }

  // 1 character GOES spacecraft indicator ('E' or 'W')
  {
    constexpr unsigned n = 1;
    ASSERT((len - nread) >= n);
    spacecraft = buf[nread];
    nread += n;
  }

  // 2 character data source code Data Source Code Table
  {
    constexpr unsigned n = 2;
    ASSERT((len - nread) >= n);
    memcpy(dataSourceCode, buf + nread, n);
    nread += n;
  }

  // 5 decimal digit message data length
  {
    constexpr unsigned n = 5;
    ASSERT((len - nread) >= n);
    dataLength = std::stoi(std::string(buf + nread, n));
    nread += n;
  }

  return nread;
}

} // namespace dcs
