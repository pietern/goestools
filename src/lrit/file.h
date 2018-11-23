#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lrit.h"

namespace lrit {

class File {
public:
  explicit File(const std::string& file);

  explicit File(const std::vector<uint8_t>& buf);

  const std::string& getName() const {
    return file_;
  }

  const std::vector<uint8_t>& getHeaderBuffer() const {
    return header_;
  }

  const HeaderMap& getHeaderMap() const {
    return m_;
  }

  template <typename H>
  bool hasHeader() const {
    return lrit::hasHeader<H>(m_);
  }

  template <typename H>
  H getHeader() const {
    return lrit::getHeader<H>(header_, m_);
  }

  std::string getTime() const;

  std::unique_ptr<std::istream> getData() const;

  std::vector<char> read() const;

protected:
  std::unique_ptr<std::istream> getDataFromFile() const;
  std::unique_ptr<std::istream> getDataFromBuffer() const;

  // If LRIT file is on disk
  std::string file_;

  // If LRIT file is in memory
  std::vector<uint8_t> buf_;

  std::vector<uint8_t> header_;
  HeaderMap m_;
  PrimaryHeader ph_;
};

} // namespace lrit
