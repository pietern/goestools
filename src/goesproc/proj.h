#pragma once

#if PROJ_VERSION_MAJOR < 4 || PROJ_VERSION_MAJOR > 5
#error "proj version 4 or 5 required"
#endif

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <proj_api.h>

class Proj {
public:
  explicit Proj(const std::vector<std::string>& args);

  explicit Proj(const std::map<std::string, std::string>& args);

  ~Proj();

  std::tuple<double, double> fwd(double lon, double lat);

  std::tuple<double, double> inv(double x, double y);

protected:
  projPJ proj_;
};
