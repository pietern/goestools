#pragma once

#include <string>
#include <vector>

namespace util {

std::vector<std::string> split(std::string in, char delim);

std::string join(const std::vector<std::string>& in, char delim);

std::string trimLeft(const std::string& in);

std::string trimRight(const std::string& in);

std::string toLower(const std::string& in);

std::string toUpper(const std::string& in);

size_t findLast(const std::string& in, char c);

} // namespace util
