#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lrit.h"

namespace LRIT {

class File {
public:
  File(const std::string& file);

  const std::string& getName() const {
    return file_;
  }

  const HeaderMap& getHeaderMap() const {
    return m_;
  }

  template <typename H>
  bool hasHeader() const {
    return LRIT::hasHeader<H>(m_);
  }

  template <typename H>
  H getHeader() const {
    return LRIT::getHeader<H>(header_, m_);
  }

  std::string getTime() const;

  std::unique_ptr<std::ifstream> getData() const;

protected:
  std::string file_;
  std::vector<uint8_t> header_;
  HeaderMap m_;
  PrimaryHeader ph_;
};

} // namespace LRIT
