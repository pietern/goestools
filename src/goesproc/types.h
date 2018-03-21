#pragma once

#include <string>

// AWIPS product identifier
struct AWIPS {
  std::string t1t2;
  std::string a1a2;
  std::string ii;
  std::string cccc;
  std::string yy;
  std::string gggg;
  std::string bbb;
  std::string nnn;
  std::string xxx;
};

// Image region (e.g. FD/Full Disk or NH/Northern Hemisphere)
struct Region {
  std::string nameShort;
  std::string nameLong;
};

// Image channel (e.g. IR/Infrared or CH02/Channel 02)
struct Channel {
  std::string nameShort;
  std::string nameLong;
};
