#include "gradient.h"

std::ostream& operator<<(std::ostream& os, const GradientPoint &p) {
  os << "Units: " << p.units << ", " <<
        "RGB(" << p.rgb[0] << "," << p.rgb[1] << "," << p.rgb[2] << "), " <<
        "HSV(" << p.hsv[0] << "," << p.hsv[1] << "," << p.hsv[2] << ")";
  return os;
}
