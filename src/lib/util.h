#pragma once

#include <ctime>
#include <sstream>
#include <vector>

std::vector<std::string> split(std::string in, char delim);

std::string trimLeft(const std::string& in);

std::string trimRight(const std::string& in);

bool parseTime(const std::string& in, struct timespec* ts);

void mkdirp(const std::string& path);

std::string toLower(const std::string& in);

std::string toUpper(const std::string& in);
