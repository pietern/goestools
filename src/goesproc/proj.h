#pragma once

#if PROJ_VERSION_MAJOR == 4
#include <proj_api.h>
#elif PROJ_VERSION_MAJOR >= 5
#include <proj.h>
#else
#error "proj version >= 4 required"
#endif

#if PROJ_VERSION_MAJOR == 4
// Forward compatibility.
double proj_torad (double angle_in_degrees);
#endif

#include <map>
#include <string>
#include <tuple>
#include <vector>

class Proj {
public:
  explicit Proj(const std::vector<std::string>& args);

  explicit Proj(const std::map<std::string, std::string>& args);

  ~Proj();

  std::tuple<double, double> fwd(double lon, double lat);

  std::tuple<double, double> inv(double x, double y);

protected:
#if PROJ_VERSION_MAJOR == 4
  projPJ proj_;
#elif PROJ_VERSION_MAJOR >= 5
  PJ *proj_;
#endif
};
