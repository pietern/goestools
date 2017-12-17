#pragma once

#include <string>
#include <vector>

class Reader {
public:
  virtual ~Reader();

  virtual size_t read(void* buf, size_t count) = 0;
};

class MultiFileReader : public Reader {
public:
  MultiFileReader(const std::vector<std::string>& files);

  virtual ~MultiFileReader();

  virtual size_t read(void* buf, size_t count);

protected:
  std::vector<std::string> files_;
  std::string current_file_;
  int fd_;

  bool next();
};
