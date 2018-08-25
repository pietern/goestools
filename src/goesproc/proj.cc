#include "proj.h"

#include <cstring>
#include <sstream>

namespace {

std::vector<std::string> toVector(
  const std::map<std::string, std::string>& args) {
  std::vector<std::string> vargs;
  vargs.reserve(args.size());
  for (const auto& arg : args) {
    std::stringstream ss;
    ss << arg.first << "=" << arg.second;
    vargs.push_back(ss.str());
  }
  return vargs;
}

std::string pj_error(std::string prefix = "proj: ") {
  std::stringstream ss;
  ss << prefix << pj_strerrno(pj_errno);
  return ss.str();
}

} // namespace

Proj::Proj(const std::vector<std::string>& args) {
  std::vector<char*> argv;
  for (const auto& arg : args) {
    argv.push_back(strdup(arg.c_str()));
  }
  proj_ = pj_init(argv.size(), argv.data());
  if (!proj_) {
    throw std::runtime_error(pj_error("proj initialization error: "));
  }
  for (auto& arg : argv) {
    free(arg);
  }
}

Proj::Proj(const std::map<std::string, std::string>& args)
  : Proj(toVector(args)) {
}

Proj::~Proj() {
  pj_free(proj_);
}

std::tuple<double, double> Proj::fwd(double lon, double lat) {
  projUV in = { lon, lat };
  projXY out = pj_fwd(in, proj_);
  return std::make_tuple<double, double>(std::move(out.u), std::move(out.v));
}

std::tuple<double, double> Proj::inv(double x, double y) {
  projXY in = { x, y };
  projUV out = pj_inv(in, proj_);
  return std::make_tuple<double, double>(std::move(out.u), std::move(out.v));
}
