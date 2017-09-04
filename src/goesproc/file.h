#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "lib/lrit.h"

class File {
public:
  File(const std::string& file);

  const std::string& getName() const {
    return file_;
  }

  template <typename H>
  H getHeader() const {
    return LRIT::getHeader<H>(header_, m_);
  }

  std::string getTime() const;

  std::ifstream getData() const;

protected:
  std::string file_;
  std::vector<uint8_t> header_;
  LRIT::HeaderMap m_;
  LRIT::PrimaryHeader ph_;
};
