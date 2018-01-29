#pragma once

#include <algorithm>
#include <ostream>

struct Area {
  int minColumn = 0;
  int maxColumn = 0;
  int minLine = 0;
  int maxLine = 0;

  int width() const {
    return maxColumn - minColumn;
  }

  int height() const {
    return maxLine - minLine;
  }

  bool empty() const {
    return width() == 0 && height() == 0;
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

std::ostream& operator<<(std::ostream& os, const Area& area);
