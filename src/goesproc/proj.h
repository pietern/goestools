#pragma once

#if PROJ_VERSION_MAJOR < 5
#error "proj version >= 5 required"
#endif

#include <string>
#include <tuple>

#include <proj.h>

class Proj {
public:
  explicit Proj(const std::string& args);

  ~Proj();

  std::tuple<double, double> fwd(double lon, double lat);

  std::tuple<double, double> inv(double x, double y);

protected:
  PJ *proj_;
};
