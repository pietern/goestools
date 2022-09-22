#include "proj.h"

#include <cstring>
#include <sstream>

namespace {

std::string pj_error(std::string prefix = "proj: ") {
  std::stringstream ss;
  ss << prefix << proj_errno_string(proj_errno(NULL));
  return ss.str();
}

} // namespace

Proj::Proj(const std::string& args) {
  proj_ = proj_create(NULL, args.c_str());
  if (!proj_) {
    throw std::runtime_error(pj_error("proj initialization error: "));
  }
}

Proj::~Proj() {
  proj_destroy(proj_);
}

std::tuple<double, double> Proj::fwd(double lon, double lat) {
  PJ_COORD in;
  in.uv = { lon, lat };
  PJ_COORD out = proj_trans(proj_, PJ_FWD, in);
  return std::make_tuple<double, double>(std::move(out.xy.x), std::move(out.xy.y));
}

std::tuple<double, double> Proj::inv(double x, double y) {
  PJ_COORD in;
  in.xy = { x, y };
  PJ_COORD out = proj_trans(proj_, PJ_INV, in);
  return std::make_tuple<double, double>(std::move(out.uv.u), std::move(out.uv.v));
}
