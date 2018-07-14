#pragma once

#include <algorithm>
#include <ostream>

struct GradientPoint {
  float units = 0;
  float rgb[3];
  float extent_rgb[2];
  float hsv[3];

  GradientPoint fromRGB(float r, float g, float b) const {
    GradientPoint out;
    out.units = units;

    // Rescale if max input range exceeds 1
    if (r > 1 || g > 1 || b > 1) {
      r /= 255;
      g /= 255;
      b /= 255;
    }

    out.rgb[0] = r;
    out.rgb[1] = g;
    out.rgb[2] = b;

    // Compute min/max RGB
    if (r < g && r < b) out.extent_rgb[0] = r;
    else if (g < r && g < b) out.extent_rgb[0] = g;
    else out.extent_rgb[0] = b;

    if (r > g && r > b) out.extent_rgb[1] = r;
    else if (g > r && g > b) out.extent_rgb[1] = g;
    else out.extent_rgb[1] = b;

    // Perform HSV conversion
    out.hsv[2] = extent_rgb[1];
    float d = extent_rgb[1] - extent_rgb[0];
    out.hsv[1] = (extent_rgb[1] <= 0 ? 0 : d / extent_rgb[1]);

    if (extent_rgb[0] == extent_rgb[1]) out.hsv[0] = 0;
    else {
      if (out.extent_rgb[1] == r) {
        out.hsv[0] = (g - b) / d + (g < b ? 6 : 0);
      } else if (out.extent_rgb[1] == rgb[1]) {
        out.hsv[0] = (b - r) / d + 2;
      } else if (out.extent_rgb[1] == rgb[2]) {
        out.hsv[0] = (r - g) / d + 4;
      }
    }

    out.hsv[0] /= 6;
    return out;
  }

  GradientPoint fromHSV(float h, float s, float v) const {
    GradientPoint out;
    out.units = units;
    out.hsv[0] = h;
    out.hsv[1] = s;
    out.hsv[2] = v;

    // Perform RGB conversion
    int i = (int) h;
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
      case 0: out.rgb[0] = v; out.rgb[1] = t; out.rgb[2] = p; break;
      case 1: out.rgb[0] = q; out.rgb[1] = v; out.rgb[2] = p; break;
      case 2: out.rgb[0] = p; out.rgb[1] = v; out.rgb[2] = t; break;
      case 3: out.rgb[0] = p; out.rgb[1] = q; out.rgb[2] = v; break;
      case 4: out.rgb[0] = t; out.rgb[1] = p; out.rgb[2] = v; break;
      case 5: out.rgb[0] = v; out.rgb[1] = p; out.rgb[2] = q; break;
    }

    // Compute min/max RGB
    if (out.rgb[0] < out.rgb[1] && out.rgb[0] < out.rgb[2]) out.extent_rgb[0] = out.rgb[0];
    else if (out.rgb[1] < out.rgb[0] && out.rgb[1] < out.rgb[2]) out.extent_rgb[0] = out.rgb[1];
    else out.extent_rgb[0] = out.rgb[2];

    if (out.rgb[0] > out.rgb[1] && out.rgb[0] > out.rgb[2]) out.extent_rgb[1] = out.rgb[0];
    else if (out.rgb[1] > out.rgb[0] && out.rgb[1] > out.rgb[2]) out.extent_rgb[1] = out.rgb[1];
    else out.extent_rgb[1] = out.rgb[2];

    return out;
  }

  inline float value() const {
    return hsv[2];
  }
};

struct Gradient {
  std::vector<GradientPoint> points;

  inline int size() const {
    return points.size();
  }

  // Search for the right place to insert a new point
  int addPoint(const GradientPoint &point) {
    auto i = points.begin();
    while (i != points.end() && i->units < point.units) i++;
    points.insert(i, point);
    return size();
  }

  void debug() const {
    for (auto i = points.begin(); i != points.end(); i++) {
      std::cerr << i->units << " " << i->rgb[0] << "," << i->rgb[1] << "," << i->rgb[2] << std::endl;
    }
  }

  // Find bounding points and perform linear interpolation
  // in HSV color space
  GradientPoint interpolate(const float units) const {
    auto left = points.end();
    auto right = points.begin();

    // Find left and right bounding points
    auto i = points.begin();
    for (i = i; i != points.end() && right == points.begin(); i++) {
      if (i->units <= units) left = i;
      if (i->units >= units) right = i;
    }


    // Clamp to ends of gradient if out of bounds
    if (left == points.end()) return *(points.begin());
    if (right == points.begin()) return *std::prev(points.end());

    auto du = right->units - left->units;
    auto pd = (units - left->units) / du;

    // Perform HSV interpolation
    /*
    auto h = pd * right->hsv[0] + (1 - pd) * left->hsv[0];
    auto s = pd * right->hsv[1] + (1 - pd) * left->hsv[1];
    auto v = pd * right->hsv[2] + (1 - pd) * left->hsv[2];
    GradientPoint out;
    out = out.fromHSV(h, s, v);
    */

    // Perform RGB interpolation
    auto r = pd * right->rgb[0] + (1 - pd) * left->rgb[0];
    auto g = pd * right->rgb[1] + (1 - pd) * left->rgb[1];
    auto b = pd * right->rgb[2] + (1 - pd) * left->rgb[2];

    // Perform color-space conversions and return complete result
    GradientPoint out;
    out = out.fromRGB(r, g, b);

    out.units = units;

    return out;
  }
};
