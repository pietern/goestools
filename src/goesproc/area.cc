#include "area.h"

std::ostream& operator<<(std::ostream& os, const Area& area) {
  int w = area.maxColumn - area.minColumn;
  int h = area.maxLine - area.minLine;
  os << "(";
  os << w << "x" << h;
  if (area.minColumn >= 0) {
    os << "+";
  }
  os << area.minColumn;
  if (area.minLine >= 0) {
    os << "+";
  }
  os << area.minLine;
  os << ")";
  return os;
}
