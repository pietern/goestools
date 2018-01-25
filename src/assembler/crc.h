#pragma once

#include <cstddef>
#include <cstdint>

namespace assembler {

uint16_t crc(const uint8_t* buf, size_t len);

} // namespace assembler
