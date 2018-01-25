#pragma once

#include <cstddef>

namespace decoder {

class Reader {
public:
  virtual ~Reader();

  virtual size_t read(void* buf, size_t count) = 0;
};

} // namespace decoder
