#include "map_drawer.h"

using namespace nlohmann;

namespace {

Proj longitudeToProj(float longitude) {
  std::map<std::string, std::string> args;
  args["proj"] = "geos";
  args["h"] = "35786023.0";
  args["lon_0"] = std::to_string(longitude);
  args["sweep"] = "x";
  return Proj(args);
}

} // namespace

MapDrawer::MapDrawer(
  const Config::Handler* config,
  float longitude,
  lrit::ImageNavigationHeader inh)
  : config_(config),
    proj_(longitudeToProj(longitude)),
    inh_(inh) {
  const auto& maps = config_->maps;
  points_.resize(maps.size());
  for (size_t i = 0; i < maps.size(); i++) {
    generatePoints(maps[i], points_[i]);
  }
}

void MapDrawer::generatePoints(
    std::vector<std::vector<cv::Point>>& out,
    const json& coords) {
  std::vector<cv::Point> points;
  double lat, lon;
  double x, y;
  for (const auto& coord : coords) {
    lon = coord.at(0).get<double>() * DEG_TO_RAD;
    lat = coord.at(1).get<double>() * DEG_TO_RAD;
    std::tie(x, y) = proj_.fwd(lon, lat);

    // If out of range, ignore
    if (fabs(x) > 1e10f || fabs(y) > 1e10f) {
      if (points.size() >= 2) {
        out.push_back(std::move(points));
      }
      points.clear();
      continue;
    }

    // This magical constant is used to scale the columnScaling and
    // lineScaling values from the LRIT image navigation header to the
    // range where they are usable with the proj projections.
    //
    // It was calculated as follows: the LRIT spec at
    // https://www.cgms-info.org/documents/pdf_cgms_03.pdf defines the
    // column and line coordinates as follows:
    //
    //   c = COFF + int(x * 2^-16 * CFAC);
    //   l = LOFF + int(y * 2^-16 * LFAC);
    //
    // We know that for the ABI full disk images on GOES-16 there is a
    // 2km per pixel resolution at nadir. The proj projections return
    // 1m per pixel resolution. To map one into the other, we can
    // simply multiply the proj projection by 0.0005 (though 0.000499
    // looks to be more accurate by visual inspection).
    //
    // With both CFAC and LFAC equal to 20425862 for the ABI full disk
    // images, we can derive our magical constant as follows:
    //
    //   k = 20425862.0 / (0.000499 * 0x10000)
    //
    // The 0x10000 divisor is removed from the computations below (and
    // multiplication added to the offsets), such that there at 16
    // fractional bits in the resulting coordinates that OpenCV can
    // use for better anti-aliasing of the lines it draws.
    //
    constexpr float k = 624597.0334223134;
    auto columnScaling = inh_.columnScaling / k;
    auto lineScaling = inh_.lineScaling / k;
    auto columnOffset = inh_.columnOffset;
    auto lineOffset = inh_.lineOffset;
    auto c = (0x10000 * columnOffset) + int(x * columnScaling);
    auto l = (0x10000 * lineOffset) - int(y * lineScaling);
    points.emplace_back(c, l);
  }

  if (points.size() >= 2) {
    out.push_back(std::move(points));
  }
}

void MapDrawer::generatePoints(const Config::Map& map, std::vector<std::vector<cv::Point>>& out) {
  // Iterate over features to aggregate line segments to draw
  for (const auto& feature : map.geo->at("features")) {
    // Expect every element to be of type "Feature"
    if (feature["type"] != "Feature") {
      continue;
    }

    const auto& geometry = feature["geometry"];
    if (geometry["type"] == "Polygon") {
      for (const auto& poly0 : geometry["coordinates"]) {
        generatePoints(out, poly0);
      }
    } else if (geometry["type"] == "MultiPolygon") {
      for (const auto& poly0 : geometry["coordinates"]) {
        for (const auto& poly1 : poly0) {
          generatePoints(out, poly1);
        }
      }
    } else if (geometry["type"] == "LineString") {
      generatePoints(out, geometry["coordinates"]);
    }
  }
}

cv::Mat MapDrawer::draw(cv::Mat& in) {
  // Convert grayscale image to color image to
  // accommodate colored map overlays.
  cv::Mat out;
  if (in.channels() == 1) {
    cv::cvtColor(in, out, cv::COLOR_GRAY2RGB);
  } else {
    out = in;
  }

  const auto& maps = config_->maps;
  for (size_t i = 0; i < maps.size(); i++) {
    cv::polylines(
      out,
      points_[i],
      false,
      maps[i].color,
      1,
      8,
      16);
  }

  return out;
}
