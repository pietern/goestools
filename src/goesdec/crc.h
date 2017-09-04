#pragma once

#include <stdint.h>
#include <unistd.h>

uint16_t crc(const uint8_t* buf, size_t len);
