#pragma once

#include <algorithm>

struct Area {
  int minColumn;
  int maxColumn;
  int minLine;
  int maxLine;

  int width() const {
    return maxColumn - minColumn;
  }

  int height() const {
    return maxLine - minLine;
  }

  Area getIntersection(const Area& other) const {
    Area result;
    result.minColumn = std::max(minColumn, other.minColumn);
    result.maxColumn = std::min(maxColumn, other.maxColumn);
    result.minLine = std::max(minLine, other.minLine);
    result.maxLine = std::min(maxLine, other.maxLine);
    return result;
  }

  Area getUnion(const Area& other) const {
    Area result;
    result.minColumn = std::min(minColumn, other.minColumn);
    result.maxColumn = std::max(maxColumn, other.maxColumn);
    result.minLine = std::min(minLine, other.minLine);
    result.maxLine = std::max(maxLine, other.maxLine);
    return result;
  }
};
