#pragma once

#include <stdint.h>
#include <unistd.h>

int correlate(uint8_t* data, size_t len, int* maxOut, int* phaseOut);
