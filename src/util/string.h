#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace util {

namespace detail {

inline std::ostream& _str(std::ostream& ss) {
  return ss;
}

template <typename T>
inline std::ostream& _str(std::ostream& ss, const T& t) {
  ss << t;
  return ss;
}

template <typename T, typename... Args>
inline std::ostream& _str(std::ostream& ss, const T& t, const Args&... args) {
  return _str(_str(ss, t), args...);
}

} // namespace detail

// Convert a list of string-like arguments into a single string.
template <typename... Args>
inline std::string str(const Args&... args) {
  std::ostringstream ss;
  detail::_str(ss, args...);
  return ss.str();
}

// Specializations for already-a-string types.
template <>
inline std::string str(const std::string& str) {
  return str;
}

inline std::string str(const char* c_str) {
  return c_str;
}

std::vector<std::string> split(std::string in, char delim);

std::string join(const std::vector<std::string>& in, char delim);

std::string trimLeft(const std::string& in);

std::string trimRight(const std::string& in);

std::string toLower(const std::string& in);

std::string toUpper(const std::string& in);

size_t findLast(const std::string& in, char c);

} // namespace util
