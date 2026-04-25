#pragma once

#include "MathUtils.h"

#include "ofxPixels.h"

#include <cmath>

namespace rimell {

template <typename T>
T *pixelAddress(const Image &image, int x, int y) {
  if (!image.data || x < image.bounds.x1 || x >= image.bounds.x2 || y < image.bounds.y1 ||
      y >= image.bounds.y2) {
    return nullptr;
  }

  auto *row = static_cast<unsigned char *>(image.data) + (y - image.bounds.y1) * image.rowBytes;
  return reinterpret_cast<T *>(row) + (x - image.bounds.x1);
}

template <typename T>
Pixel readPixelTyped(const T *pixel);

template <>
inline Pixel readPixelTyped<OfxRGBAColourB>(const OfxRGBAColourB *pixel) {
  if (!pixel) {
    return {};
  }
  constexpr float scale = 1.0f / 255.0f;
  return {pixel->r * scale, pixel->g * scale, pixel->b * scale, pixel->a * scale};
}

template <>
inline Pixel readPixelTyped<OfxRGBAColourS>(const OfxRGBAColourS *pixel) {
  if (!pixel) {
    return {};
  }
  constexpr float scale = 1.0f / 65535.0f;
  return {pixel->r * scale, pixel->g * scale, pixel->b * scale, pixel->a * scale};
}

template <>
inline Pixel readPixelTyped<OfxRGBAColourF>(const OfxRGBAColourF *pixel) {
  if (!pixel) {
    return {};
  }
  return {pixel->r, pixel->g, pixel->b, pixel->a};
}

template <typename T>
void writePixelTyped(T *pixel, const Pixel &value);

template <>
inline void writePixelTyped<OfxRGBAColourB>(OfxRGBAColourB *pixel, const Pixel &value) {
  pixel->r = static_cast<unsigned char>(std::lround(clamp01(value.r) * 255.0f));
  pixel->g = static_cast<unsigned char>(std::lround(clamp01(value.g) * 255.0f));
  pixel->b = static_cast<unsigned char>(std::lround(clamp01(value.b) * 255.0f));
  pixel->a = static_cast<unsigned char>(std::lround(clamp01(value.a) * 255.0f));
}

template <>
inline void writePixelTyped<OfxRGBAColourS>(OfxRGBAColourS *pixel, const Pixel &value) {
  pixel->r = static_cast<unsigned short>(std::lround(clamp01(value.r) * 65535.0f));
  pixel->g = static_cast<unsigned short>(std::lround(clamp01(value.g) * 65535.0f));
  pixel->b = static_cast<unsigned short>(std::lround(clamp01(value.b) * 65535.0f));
  pixel->a = static_cast<unsigned short>(std::lround(clamp01(value.a) * 65535.0f));
}

template <>
inline void writePixelTyped<OfxRGBAColourF>(OfxRGBAColourF *pixel, const Pixel &value) {
  pixel->r = value.r;
  pixel->g = value.g;
  pixel->b = value.b;
  pixel->a = value.a;
}

template <typename T>
Pixel sampleNearest(const Image &image, float x, float y) {
  if (!image.data || image.bounds.x1 >= image.bounds.x2 || image.bounds.y1 >= image.bounds.y2) {
    return {};
  }

  const int ix = clampCoord(static_cast<int>(std::floor(x + 0.5f)), image.bounds.x1, image.bounds.x2 - 1);
  const int iy = clampCoord(static_cast<int>(std::floor(y + 0.5f)), image.bounds.y1, image.bounds.y2 - 1);
  return readPixelTyped(pixelAddress<T>(image, ix, iy));
}

template <typename T>
Pixel sampleBilinear(const Image &image, float x, float y) {
  if (!image.data || image.bounds.x1 >= image.bounds.x2 || image.bounds.y1 >= image.bounds.y2) {
    return {};
  }

  const int x0 = static_cast<int>(std::floor(x));
  const int y0 = static_cast<int>(std::floor(y));
  const float tx = x - static_cast<float>(x0);
  const float ty = y - static_cast<float>(y0);

  const int x0c = clampCoord(x0, image.bounds.x1, image.bounds.x2 - 1);
  const int x1c = clampCoord(x0 + 1, image.bounds.x1, image.bounds.x2 - 1);
  const int y0c = clampCoord(y0, image.bounds.y1, image.bounds.y2 - 1);
  const int y1c = clampCoord(y0 + 1, image.bounds.y1, image.bounds.y2 - 1);

  const Pixel p00 = readPixelTyped(pixelAddress<T>(image, x0c, y0c));
  const Pixel p10 = readPixelTyped(pixelAddress<T>(image, x1c, y0c));
  const Pixel p01 = readPixelTyped(pixelAddress<T>(image, x0c, y1c));
  const Pixel p11 = readPixelTyped(pixelAddress<T>(image, x1c, y1c));

  const Pixel top = lerpPixel(p00, p10, tx);
  const Pixel bottom = lerpPixel(p01, p11, tx);
  return lerpPixel(top, bottom, ty);
}

} // namespace rimell
