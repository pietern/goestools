#pragma once

#include <ctime>
#include <string>

namespace util {

std::string stringTime();

bool parseTime(const std::string& in, struct timespec* ts);

} // namespace util
