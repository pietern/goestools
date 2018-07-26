#pragma once

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

// FileWriter writes files to disk.
// This is where overwrite logic and logging is handled.
class FileWriter {
public:
  explicit FileWriter(const std::string& prefix);
  ~FileWriter();

  void setForce(bool force) {
    force_ = force;
  }

  void write(const std::string& path, const cv::Mat& mat);

  void write(const std::string& path, const std::vector<char>& data);

protected:
  bool tryWrite(const std::string& path);

  std::string buildPath(const std::string& path);

  const std::string prefix_;
  bool force_;
};
