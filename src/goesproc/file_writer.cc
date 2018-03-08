#include "file_writer.h"

#include <sys/stat.h>

#include <fstream>

FileWriter::FileWriter() {
  force_ = false;
}

FileWriter::~FileWriter() {
}

void FileWriter::write(const std::string& path, const cv::Mat& mat) {
  if (!tryWrite(path)) {
    std::cerr << "Skipping (file exists): " << path << std::endl;
    return;
  }

  std::cerr << "Writing: " << path << std::endl;
  cv::imwrite(path, mat);
}

void FileWriter::write(const std::string& path, const std::vector<char>& data) {
  if (!tryWrite(path)) {
    std::cerr << "Skipping (file exists): " << path << std::endl;
    return;
  }

  std::cerr << "Writing: " << path << std::endl;
  std::ofstream of(path);
  of.write(data.data(), data.size());
}

bool FileWriter::tryWrite(const std::string& path) {
  struct stat st;
  auto rv = stat(path.c_str(), &st);
  if (rv < 0 && errno == ENOENT) {
    return true;
  }

  return force_;
}
