#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

struct GradientPoint {
  float units = 0;
  float rgb[3];
  float hsv[3];

  static const GradientPoint fromRGB(float u, float r, float g, float b) {
    float min, max;
    GradientPoint out;
    out.units = u;

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
    if (r <= g && r <= b) min = r;
    else if (g <= r && g <= b) min = g;
    else min = b;

    if (r >= g && r >= b) max = r;
    else if (g >= r && g >= b) max = g;
    else max = b;

    // Perform RGB->HSV conversion
    out.hsv[2] = max;
    float d = max - min;
    out.hsv[1] = (max <= 0) ? 0 : d / max;

    if (min == max) out.hsv[0] = 0;
    else {
      if (max == r) out.hsv[0] = fmod((g - b) / d, 6);
      else if (max == g) out.hsv[0] = (b - r) / d + 2;
      else if (max == b) out.hsv[0] = (r - g) / d + 4;
    }

    out.hsv[0] /= 6;
    return out;
  }

  static const GradientPoint fromHSV(float u, float h, float s, float v) {
    GradientPoint out;
    out.units = u;

    // Rescale if max input range exceeds 1
    if (h > 1 || s > 1 || v > 1) {
      h /= 360;
      s /= 100;
      v /= 100;
    }

    out.hsv[0] = h;
    out.hsv[1] = s;
    out.hsv[2] = v;

    // Perform HSV->RGB conversion
    int i = (int) (h * 6);
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

    return out;
  }
};

std::ostream& operator<<(std::ostream& os, const GradientPoint &p);

enum GradientInterpolationType {
  LERP_UNDEFINED = -1,
  LERP_RGB,
  LERP_HSV,
  LERP_NUMTYPES
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

  // Emit gradient points to stderr for debug purposes
  void debug() const {
    for (auto i = points.begin(); i != points.end(); i++) {
      std::cerr << "[" << (i - points.begin()) << "] -> " << *i << std::endl;
    }
  }

  // Find bounding points and perform linear interpolation
  const GradientPoint interpolate(const float units, const GradientInterpolationType lerptype = LERP_UNDEFINED) const {
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

    // Compute parametric distance between endpoints
    auto du = right->units - left->units;
    auto pd = (units - left->units) / du;

    GradientPoint out;


    // Default to RGB interpolation if not specified
    if (lerptype == LERP_RGB ||
        lerptype == LERP_UNDEFINED) {
      float r = pd * right->rgb[0] + (1 - pd) * left->rgb[0];
      float g = pd * right->rgb[1] + (1 - pd) * left->rgb[1];
      float b = pd * right->rgb[2] + (1 - pd) * left->rgb[2];

      // Perform color-space conversion
      out = GradientPoint::fromRGB(units, r, g, b);
    } else if (lerptype == LERP_HSV) {
      auto lh = left->hsv[0], rh = right->hsv[0];
      double dh = rh - lh, adh = fabs(dh);

      // Flip direction to choose shortest path in hue dimension
      if (adh > 0.5) rh -= dh / adh;

      float h = pd * rh + (1 - pd) * lh;
      float s = pd * right->hsv[1] + (1 - pd) * left->hsv[1];
      float v = pd * right->hsv[2] + (1 - pd) * left->hsv[2];

      // Wrap the hue interpolation to deal with any negative values
      if (h < 0.0) h += 1.0;

      // Perform color-space conversion
      out = GradientPoint::fromHSV(units, h, s, v);
    } else {
      throw std::runtime_error("Invalid interpolation type specified");
    }

    return out;
  }
};
