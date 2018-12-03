#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

#include "lib/timer.h"
#include "lrit/file.h"

// FileWriter writes files to disk.
// This is where overwrite logic and logging is handled.
class FileWriter {
public:
  explicit FileWriter(const std::string& prefix);
  ~FileWriter();

  void setForce(bool force) {
    force_ = force;
  }

  void write(
    const std::string& path,
    const cv::Mat& mat,
    const Timer* t = nullptr);

  void write(
    const std::string& path,
    const std::vector<char>& data,
    const Timer* t = nullptr);

  void write(
    const std::string& path,
    const nlohmann::json& json,
    const Timer* t = nullptr);

  void writeHeader(
    const lrit::File& file,
    const std::string& path);

protected:
  bool tryWrite(const std::string& path);

  std::string buildPath(const std::string& path);

  void logTime(const Timer* t);

  const std::string prefix_;
  bool force_;
};
