#pragma once

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

// FileWriter writes files to disk.
// This is where overwrite logic and logging is handled.
class FileWriter {
public:
  explicit FileWriter();
  ~FileWriter();

  void setForce(bool force) {
    force_ = force;
  }

  void write(const std::string& path, const cv::Mat& mat);

  void write(const std::string& path, const std::vector<char>& data);

protected:
  bool tryWrite(const std::string& path);

  bool force_;
};
