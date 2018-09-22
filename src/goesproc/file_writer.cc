#include "file_writer.h"

#include <sys/stat.h>

#include <fstream>
#include <iomanip>

#include "lib/util.h"

FileWriter::FileWriter(const std::string& prefix) : prefix_(prefix) {
  force_ = false;
}

FileWriter::~FileWriter() {
}

void FileWriter::logTime(const Timer* t) {
  if (t) {
    std::cout
      << std::fixed
      << std::setprecision(3)
      << " (took "
      << t->elapsed().count()
      << "s)"
      << std::endl;
  } else {
    std::cout << std::endl;
  }
}

void FileWriter::write(
  const std::string& tail,
  const cv::Mat& mat,
  const Timer* t) {
  auto path = buildPath(tail);
  if (!tryWrite(path)) {
    std::cout << "Skipping (file exists): " << path;
    logTime(t);
    return;
  }

  std::cout << "Writing: " << path;
  cv::imwrite(path, mat);
  logTime(t);
}

void FileWriter::write(
  const std::string& tail,
  const std::vector<char>& data,
  const Timer* t) {
  auto path = buildPath(tail);
  if (!tryWrite(path)) {
    std::cout << "Skipping (file exists): " << path;
    logTime(t);
    return;
  }

  std::cout << "Writing: " << path;
  std::ofstream of(path);
  of.write(data.data(), data.size());
  logTime(t);
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
