#include "file_writer.h"

#include <sys/stat.h>

#include <fstream>

#include "lib/util.h"

FileWriter::FileWriter(const std::string& prefix) : prefix_(prefix) {
  force_ = false;
}

FileWriter::~FileWriter() {
}

void FileWriter::write(const std::string& tail, const cv::Mat& mat) {
  auto path = buildPath(tail);
  if (!tryWrite(path)) {
    std::cout << "Skipping (file exists): " << path << std::endl;
    return;
  }

  std::cout << "Writing: " << path << std::endl;
  cv::imwrite(path, mat);
}

void FileWriter::write(const std::string& tail, const std::vector<char>& data) {
  auto path = buildPath(tail);
  if (!tryWrite(path)) {
    std::cout << "Skipping (file exists): " << path << std::endl;
    return;
  }

  std::cout << "Writing: " << path << std::endl;
  std::ofstream of(path);
  of.write(data.data(), data.size());
}

bool FileWriter::tryWrite(const std::string& path) {
  struct stat st;

  auto rpos = path.rfind('/');
  if (rpos != std::string::npos) {
    mkdirp(path.substr(0, rpos));
  }

  auto rv = stat(path.c_str(), &st);
  if (rv < 0 && errno == ENOENT) {
    return true;
  }

  return force_;
}

std::string FileWriter::buildPath(const std::string& path) {
  if (prefix_ == ".") {
    return path;
  }
  return prefix_ + "/" + path;
}
