#pragma once

#include "Types.h"

#include <algorithm>
#include <cmath>

namespace rimell {

inline float clamp01(float value) {
  return std::max(0.0f, std::min(1.0f, value));
}

inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

inline Pixel lerpPixel(const Pixel &a, const Pixel &b, float t) {
  return {
      lerp(a.r, b.r, t),
      lerp(a.g, b.g, t),
      lerp(a.b, b.b, t),
      lerp(a.a, b.a, t),
  };
}

inline float luminance(const Pixel &p) {
  return 0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b;
}

inline float smoothstep(float edge0, float edge1, float x) {
  if (edge0 == edge1) {
    return x < edge0 ? 0.0f : 1.0f;
  }
  const float t = clamp01((x - edge0) / (edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

inline int clampCoord(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

} // namespace rimell
