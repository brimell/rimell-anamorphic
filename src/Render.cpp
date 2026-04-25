#include "Render.h"

#include "Constants.h"
#include "Diagnostics.h"
#include "HostSuites.h"
#include "LensMap.h"
#include "MetalRender.h"
#include "Parameters.h"
#include "PixelAccess.h"
#include "RenderCore.h"

#include "ofxPixels.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <string>

namespace rimell {
namespace {

struct DepthAlphaB {
  unsigned char v;
};

struct DepthAlphaS {
  unsigned short v;
};

struct DepthAlphaF {
  float v;
};

enum class DepthBitDepth { Byte, Short, Float };

enum class DepthComponents { Alpha, RGBA };

enum class DebugView {
  Off = 0,
  Source = 1,
  Depth = 2,
  DepthFocusMask = 3,
  HighlightMatte = 4,
  EdgeMask = 5,
};

struct DepthImage {
  const Image *image = nullptr;
  DepthBitDepth bitDepth = DepthBitDepth::Float;
  DepthComponents components = DepthComponents::Alpha;
};

template <typename T>
float sampleDepthAlphaBilinear(const Image &image, float x, float y, float scale) {
  if (!image.data || image.bounds.x1 >= image.bounds.x2 || image.bounds.y1 >= image.bounds.y2 ||
      !std::isfinite(x) || !std::isfinite(y)) {
    return 0.0f;
  }

  const int x0 = static_cast<int>(std::floor(x));
  const int y0 = static_cast<int>(std::floor(y));
  const float tx = x - static_cast<float>(x0);
  const float ty = y - static_cast<float>(y0);

  const int x0c = clampCoord(x0, image.bounds.x1, image.bounds.x2 - 1);
  const int x1c = clampCoord(x0 + 1, image.bounds.x1, image.bounds.x2 - 1);
  const int y0c = clampCoord(y0, image.bounds.y1, image.bounds.y2 - 1);
  const int y1c = clampCoord(y0 + 1, image.bounds.y1, image.bounds.y2 - 1);

  const T *p00 = pixelAddress<T>(image, x0c, y0c);
  const T *p10 = pixelAddress<T>(image, x1c, y0c);
  const T *p01 = pixelAddress<T>(image, x0c, y1c);
  const T *p11 = pixelAddress<T>(image, x1c, y1c);

  const float v00 = p00 ? static_cast<float>(p00->v) * scale : 0.0f;
  const float v10 = p10 ? static_cast<float>(p10->v) * scale : 0.0f;
  const float v01 = p01 ? static_cast<float>(p01->v) * scale : 0.0f;
  const float v11 = p11 ? static_cast<float>(p11->v) * scale : 0.0f;

  const float top = lerp(v00, v10, tx);
  const float bottom = lerp(v01, v11, tx);
  return lerp(top, bottom, ty);
}

template <typename T>
Pixel warpedSourceSample(const Image &source, float dstX, float dstY, int width, int height,
                         const RenderParams &params, float caPixels) {
  const Vec2 cropped = applyEdgeCrop(dstX, dstY, source, params.edgeCropScale);
  const LensMap map = buildLensMap(cropped.x, cropped.y, source, width, height, params);
  Vec2 sourcePixel = lensMapToSourcePixel(map, source, width, height);
  const float caMask = params.edgeOnlyCA > 0.5f ? smoothstep(0.2f, 1.0f, map.radius) : 1.0f;
  sourcePixel.x += map.caDirection.x * caPixels * caMask;
  sourcePixel.y += map.caDirection.y * caPixels * caMask;
  return sampleBilinear<T>(source, sourcePixel.x, sourcePixel.y);
}

bool parseDepthBitDepth(const char *bitDepth, DepthBitDepth *outDepthBitDepth) {
  if (!bitDepth || !outDepthBitDepth) {
    return false;
  }
  if (std::strcmp(bitDepth, kOfxBitDepthByte) == 0) {
    *outDepthBitDepth = DepthBitDepth::Byte;
    return true;
  }
  if (std::strcmp(bitDepth, kOfxBitDepthShort) == 0) {
    *outDepthBitDepth = DepthBitDepth::Short;
    return true;
  }
  if (std::strcmp(bitDepth, kOfxBitDepthFloat) == 0) {
    *outDepthBitDepth = DepthBitDepth::Float;
    return true;
  }
  return false;
}

bool parseDepthComponents(const char *components, DepthComponents *outDepthComponents) {
  if (!components || !outDepthComponents) {
    return false;
  }
  if (std::strcmp(components, kOfxImageComponentAlpha) == 0) {
    *outDepthComponents = DepthComponents::Alpha;
    return true;
  }
  if (std::strcmp(components, kOfxImageComponentRGBA) == 0) {
    *outDepthComponents = DepthComponents::RGBA;
    return true;
  }
  return false;
}

int bytesPerChannel(DepthBitDepth bitDepth) {
  switch (bitDepth) {
  case DepthBitDepth::Byte:
    return 1;
  case DepthBitDepth::Short:
    return 2;
  case DepthBitDepth::Float:
    return 4;
  }
  return 0;
}

bool validateImageLayout(const Image &image, int bytesPerPixel) {
  const int width = image.bounds.x2 - image.bounds.x1;
  const int height = image.bounds.y2 - image.bounds.y1;
  if (!image.data || image.rowBytes == 0 || width <= 0 || height <= 0 || bytesPerPixel <= 0) {
    return false;
  }

  const int absRowBytes = image.rowBytes < 0 ? -image.rowBytes : image.rowBytes;
  const long long required = static_cast<long long>(width) * static_cast<long long>(bytesPerPixel);
  return static_cast<long long>(absRowBytes) >= required;
}

void logImageLayout(LogLevel level, const char *scope, const char *name, const Image &image,
                    int bytesPerPixel, bool valid) {
  const int width = image.bounds.x2 - image.bounds.x1;
  const int height = image.bounds.y2 - image.bounds.y1;
  const int absRowBytes = image.rowBytes < 0 ? -image.rowBytes : image.rowBytes;
  const long long required = static_cast<long long>(width) * static_cast<long long>(bytesPerPixel);
  logPrintf(level,
            scope,
            "%s layout valid=%d data=%p bounds=[%d,%d,%d,%d] size=%dx%d rowBytes=%d absRowBytes=%d bpp=%d minRowBytes=%lld",
            name ? name : "image",
            valid ? 1 : 0,
            image.data,
            image.bounds.x1,
            image.bounds.y1,
            image.bounds.x2,
            image.bounds.y2,
            width,
            height,
            image.rowBytes,
            absRowBytes,
            bytesPerPixel,
            required);
}

float sampleDepthAt(const DepthImage *depth, float x, float y, const RenderParams &params) {
  if (!depth || !depth->image || !depth->image->data || params.depthMapEnabled == 0) {
    return 0.0f;
  }

  if (!std::isfinite(x) || !std::isfinite(y)) {
    return 0.0f;
  }

  float value = 0.0f;
  if (depth->components == DepthComponents::Alpha) {
    if (depth->bitDepth == DepthBitDepth::Byte) {
      value = sampleDepthAlphaBilinear<DepthAlphaB>(*depth->image, x, y, 1.0f / 255.0f);
    } else if (depth->bitDepth == DepthBitDepth::Short) {
      value = sampleDepthAlphaBilinear<DepthAlphaS>(*depth->image, x, y, 1.0f / 65535.0f);
    } else {
      value = sampleDepthAlphaBilinear<DepthAlphaF>(*depth->image, x, y, 1.0f);
    }
  } else {
    Pixel sample{};
    if (depth->bitDepth == DepthBitDepth::Byte) {
      sample = sampleBilinear<OfxRGBAColourB>(*depth->image, x, y);
    } else if (depth->bitDepth == DepthBitDepth::Short) {
      sample = sampleBilinear<OfxRGBAColourS>(*depth->image, x, y);
    } else {
      sample = sampleBilinear<OfxRGBAColourF>(*depth->image, x, y);
    }
    value = luminance(sample);
  }

  if (!std::isfinite(value)) {
    return 0.0f;
  }

  value = clamp01(value);
  return params.depthMapInvert != 0 ? 1.0f - value : value;
}

float depthDefocusMaskAt(const DepthImage *depth, float x, float y, const RenderParams &params) {
  if (!depth || !depth->image || !depth->image->data || params.depthMapEnabled == 0 ||
      params.depthInfluence <= 0.0001f) {
    return 0.0f;
  }

  const float depthValue = sampleDepthAt(depth, x, y, params);
  const float separation = std::abs(depthValue - params.focusDepth);
  const float focusRange = std::max(0.001f, params.depthFocusRange);
  const float feather = std::max(0.05f, focusRange * 0.75f);
  return smoothstep(focusRange, focusRange + feather, separation) * params.depthInfluence;
}

float highlightMatteAt(const Pixel &sample, const RenderParams &params) {
  return clamp01(smoothstep(params.flareThreshold, 1.0f, luminance(sample)));
}

float edgeMaskAt(const Image &source, float x, float y, int width, int height,
                 const RenderParams &params, const DepthImage *depth) {
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
  const LensMap map = buildLensMap(x, y, source, width, height, params);
  const float radius = std::max(map.radius, std::sqrt(nx * nx + ny * ny));
  const float depthDefocus = depthDefocusMaskAt(depth, x, y, params);
  const float edge = std::max({map.edgeMask,
                               smoothstep(std::max(0.0f, 1.0f - params.radialFalloff), 1.15f, radius),
                               depthDefocus * 0.7f});
  return clamp01(edge);
}

float qualityScale(const RenderParams &params) {
  switch (params.renderQuality) {
  case 0:
    return 0.5f;
  case 2:
    return 1.5f;
  default:
    return 1.0f;
  }
}

template <typename T>
Pixel channelDefocusSample(const Image &source, float x, float y, int width, int height,
                           const RenderParams &params, float radiusPixels) {
  if (radiusPixels <= 0.05f) {
    return warpedSourceSample<T>(source, x, y, width, height, params, 0.0f);
  }

  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  float dx = x - cx;
  float dy = y - cy;
  const float length = std::sqrt(dx * dx + dy * dy);
  if (length > 0.0001f) {
    dx /= length;
    dy /= length;
  } else {
    dx = 1.0f;
    dy = 0.0f;
  }

  const Pixel center = warpedSourceSample<T>(source, x, y, width, height, params, 0.0f);
  const Pixel outward = warpedSourceSample<T>(source, x + dx * radiusPixels, y + dy * radiusPixels, width,
                                             height, params, 0.0f);
  const Pixel inward = warpedSourceSample<T>(source, x - dx * radiusPixels, y - dy * radiusPixels, width,
                                            height, params, 0.0f);
  Pixel result{};
  result.r = center.r * 0.5f + (outward.r + inward.r) * 0.25f;
  result.g = center.g * 0.5f + (outward.g + inward.g) * 0.25f;
  result.b = center.b * 0.5f + (outward.b + inward.b) * 0.25f;
  result.a = center.a;
  return result;
}

template <typename T>
Pixel opticalBaseSample(const Image &source, float x, float y, int width, int height,
                        const RenderParams &params, const DepthImage *depth) {
  const float caPixels = params.lateralCA * params.lateralCAPixelScale;
  Pixel base = warpedSourceSample<T>(source, x, y, width, height, params, 0.0f);
  Pixel red = warpedSourceSample<T>(source, x, y, width, height, params, caPixels);
  Pixel blue = warpedSourceSample<T>(source, x, y, width, height, params, -caPixels);
  base.r = red.r;
  base.b = blue.b;

  if (params.longitudinalCA > 0.001f) {
    const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
    const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
    const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
    const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
    const float edge = smoothstep(0.15f, 1.05f, std::sqrt(nx * nx + ny * ny));
    const float focusBias = 0.35f + std::abs(params.focusDistance - 0.5f) * 1.3f;
    const float depthDefocus = depthDefocusMaskAt(depth, x, y, params);
    const float radiusPixels = params.longitudinalCA * focusBias * std::max(edge, depthDefocus) * 4.0f;
    const Pixel redDefocus = channelDefocusSample<T>(source, x, y, width, height, params, radiusPixels);
    const Pixel blueDefocus = channelDefocusSample<T>(source, x, y, width, height, params, radiusPixels * 0.65f);
    const float amount = clamp01(params.longitudinalCA * (0.35f + std::max(edge, depthDefocus) * 0.65f));
    base.r = lerp(base.r, redDefocus.r, amount);
    base.b = lerp(base.b, blueDefocus.b, amount);
  }

  return base;
}

template <typename T>
Pixel edgeCharacter(const Image &source, float x, float y, int width, int height, const Pixel &base,
                    const RenderParams &params, const DepthImage *depth) {
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
  const LensMap map = buildLensMap(x, y, source, width, height, params);
  const float radius = std::max(map.radius, std::sqrt(nx * nx + ny * ny));
  const float depthDefocus = depthDefocusMaskAt(depth, x, y, params);
  const float edge = std::max({map.edgeMask,
                               smoothstep(std::max(0.0f, 1.0f - params.radialFalloff), 1.15f, radius),
                               depthDefocus * 0.7f});

  Pixel result = base;

  const float blurRadius =
      params.edgeBlur * edge * params.edgeBlurPixels + params.fieldCurvature * edge * params.fieldCurvaturePixels +
      depthDefocus * params.depthDefocusPixels * 0.35f;
  if (blurRadius > 0.05f) {
    Pixel blur{};
    float weight = 0.0f;
    const int blurSamples = params.renderQuality == 0 ? 2 : (params.renderQuality == 2 ? 4 : 3);
    for (int i = -blurSamples; i <= blurSamples; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(blurSamples);
      const float w = 1.0f - std::abs(t) * 0.55f;
      const Pixel sample = warpedSourceSample<T>(source, x + t * blurRadius, y + t * blurRadius * 0.25f, width,
                                                 height, params, 0.0f);
      blur.r += sample.r * w;
      blur.g += sample.g * w;
      blur.b += sample.b * w;
      blur.a += sample.a * w;
      weight += w;
    }
    blur.r /= weight;
    blur.g /= weight;
    blur.b /= weight;
    blur.a /= weight;
    result = lerpPixel(result, blur, clamp01(edge * (params.edgeBlur + params.fieldCurvature)));
  }

  const float smearRadius = edge * (params.tangentialSmear + params.horizontalSmear) * params.smearPixels;
  if (smearRadius > 0.05f) {
    Pixel smear{};
    float weight = 0.0f;
    const int smearSamples = params.renderQuality == 0 ? 2 : (params.renderQuality == 2 ? 5 : 4);
    for (int i = -smearSamples; i <= smearSamples; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(smearSamples);
      const float w = 1.0f - std::abs(t) * 0.7f;
      const Pixel sample = warpedSourceSample<T>(source, x + t * smearRadius, y, width, height, params, 0.0f);
      smear.r += sample.r * w;
      smear.g += sample.g * w;
      smear.b += sample.b * w;
      smear.a += sample.a * w;
      weight += w;
    }
    smear.r /= weight;
    smear.g /= weight;
    smear.b /= weight;
    smear.a /= weight;
    result = lerpPixel(result, smear, clamp01(edge * (params.tangentialSmear + params.horizontalSmear)));
  }

  if (params.verticalSharpness > 0.001f) {
    const Pixel up = warpedSourceSample<T>(source, x, y - 1.5f, width, height, params, 0.0f);
    const Pixel down = warpedSourceSample<T>(source, x, y + 1.5f, width, height, params, 0.0f);
    const float sharpen = params.verticalSharpness * (1.0f - edge * 0.5f);
    result.g = clamp01(result.g + (result.g - (up.g + down.g) * 0.5f) * sharpen);
    result.r = clamp01(result.r + (result.r - (up.r + down.r) * 0.5f) * sharpen * 0.5f);
    result.b = clamp01(result.b + (result.b - (up.b + down.b) * 0.5f) * sharpen * 0.5f);
  }

  return result;
}

template <typename T>
Pixel depthDefocusCharacter(const Image &source, float x, float y, int width, int height, const Pixel &base,
                            const RenderParams &params, const DepthImage *depth) {
  const float depthDefocus = depthDefocusMaskAt(depth, x, y, params);
  const float blurRadius = depthDefocus * params.depthDefocusPixels;
  if (blurRadius <= 0.05f) {
    return base;
  }

  const float rotation = params.bokehRotation * kPi / 180.0f;
  const float cosR = std::cos(rotation);
  const float sinR = std::sin(rotation);
  const float stretch = (1.0f + params.bokehStretch * params.bokehStretchScale) *
                        lensIdentityBloomScale(params);
  const int rings = params.renderQuality == 0 ? 1 : (params.renderQuality == 2 ? 3 : 2);
  const int samplesPerRing = params.renderQuality == 0 ? 5 : (params.renderQuality == 2 ? 12 : 8);

  Pixel blur{};
  float total = 0.0f;
  for (int ring = 1; ring <= rings; ++ring) {
    const float ringRadius = blurRadius * static_cast<float>(ring) / static_cast<float>(rings);
    for (int i = 0; i < samplesPerRing; ++i) {
      const float a = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(samplesPerRing);
      float ox = std::cos(a) * ringRadius / stretch;
      float oy = std::sin(a) * ringRadius * stretch;
      const float rx = ox * cosR - oy * sinR;
      const float ry = ox * sinR + oy * cosR;
      const Pixel sample = warpedSourceSample<T>(source, x + rx, y + ry, width, height, params, 0.0f);
      const float weight = 1.0f / static_cast<float>(ring);
      blur.r += sample.r * weight;
      blur.g += sample.g * weight;
      blur.b += sample.b * weight;
      blur.a += sample.a * weight;
      total += weight;
    }
  }

  if (total <= 0.0f) {
    return base;
  }

  blur.r /= total;
  blur.g /= total;
  blur.b /= total;
  blur.a /= total;
  return lerpPixel(base, blur, clamp01(depthDefocus));
}

template <typename T>
Pixel mappedSample(const Image &source, float x, float y, int width, int height, const RenderParams &params) {
  return warpedSourceSample<T>(source, x, y, width, height, params, 0.0f);
}

template <typename T>
float highlightAt(const Image &source, float x, float y, int width, int height, const RenderParams &params,
                  float threshold) {
  const Pixel p = mappedSample<T>(source, x, y, width, height, params);
  return smoothstep(threshold, 1.0f, luminance(p));
}

template <typename T>
Pixel lensAdditives(const Image &source, float x, float y, int width, int height, const RenderParams &params,
                    const Pixel &base, const DepthImage *depth) {
  Pixel add{};

  const float flareAngle = params.flareAngle * kPi / 180.0f;
  const float dirX = std::cos(flareAngle);
  const float dirY = std::sin(flareAngle);
  const float sampleScale = qualityScale(params);
  const int flareSteps =
      std::max(1, static_cast<int>(std::round((2.0f + params.flareLength * params.flareStepDensity) * sampleScale)));
  const float flareSpan = params.flareLength * static_cast<float>(width) * params.flareSpanScale *
                          lensIdentityFlareScale(params);
  if (params.flareIntensity > 0.001f && flareSpan > 1.0f) {
    for (int i = -flareSteps; i <= flareSteps; ++i) {
      if (i == 0) {
        continue;
      }
      const float t = static_cast<float>(i) / static_cast<float>(flareSteps);
      const float sx = x + dirX * t * flareSpan;
      const float sy = y + dirY * t * flareSpan;
      const float h = highlightAt<T>(source, sx, sy, width, height, params, params.flareThreshold);
      const float depthBoost = 1.0f + depthDefocusMaskAt(depth, sx, sy, params) * params.depthBloomBoost;
      const float w = std::exp(-std::abs(t) * params.flareFalloff) * h * params.flareIntensity * depthBoost;
      add.r += params.flareColour.r * w;
      add.g += params.flareColour.g * w;
      add.b += params.flareColour.b * w;
    }
  }

  const float bloomPixels = params.bloomRadius * params.bloomPixelScale;
  if ((params.veil > 0.001f || params.highlightCream > 0.001f) && bloomPixels > 0.5f) {
    const float depthDefocus = depthDefocusMaskAt(depth, x, y, params);
    const float rotation = params.bokehRotation * kPi / 180.0f;
    const float cosR = std::cos(rotation);
    const float sinR = std::sin(rotation);
    const float stretch = (1.0f + params.bokehStretch * params.bokehStretchScale) *
                          lensIdentityBloomScale(params);
    const int rings = std::max(1, static_cast<int>(std::round(static_cast<float>(params.bloomRings) * sampleScale)));
    const int samplesPerRing =
        std::max(3, static_cast<int>(std::round(static_cast<float>(params.bloomSamplesPerRing) * sampleScale)));
    float total = 0.0f;
    Pixel bloom{};
    for (int ring = 1; ring <= rings; ++ring) {
      const float ringRadius = bloomPixels * (1.0f + depthDefocus * params.depthBloomBoost) *
                               static_cast<float>(ring) / static_cast<float>(rings);
      for (int i = 0; i < samplesPerRing; ++i) {
        const float a = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(samplesPerRing);
        float ox = std::cos(a) * ringRadius / stretch;
        float oy = std::sin(a) * ringRadius * stretch;
        const float rx = ox * cosR - oy * sinR;
        const float ry = ox * sinR + oy * cosR;
        const Pixel sample = mappedSample<T>(source, x + rx, y + ry, width, height, params);
        const float h = smoothstep(params.flareThreshold * params.bloomThresholdScale, 1.0f, luminance(sample));
        const float w = h / static_cast<float>(ring);
        bloom.r += sample.r * w;
        bloom.g += sample.g * w;
        bloom.b += sample.b * w;
        total += w;
      }
    }
    if (total > 0.0f) {
      bloom.r /= total;
      bloom.g /= total;
      bloom.b /= total;
      const LensMap map = buildLensMap(x, y, source, width, height, params);
      const float edge = std::max(map.edgeMask, smoothstep(0.35f, 1.1f, map.radius));
      const float bokehEdgeKeep = 1.0f - edge * params.bokehEdgeFalloff * params.bloomEdgeKeepScale;
      const float protect = lerp(1.0f, clamp01(luminance(base) * 2.0f), params.blackLiftProtection);
      const float amount =
          (params.veil * params.bloomVeilScale + params.highlightCream * params.bloomCreamScale) * protect *
          bokehEdgeKeep * (1.0f + depthDefocus * params.depthBloomBoost);
      add.r += bloom.r * amount;
      add.g += bloom.g * amount;
      add.b += bloom.b * amount;
    }
  }

  if (params.ghostCount > 0 && params.ghostSpread > 0.001f) {
    const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
    const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
    const float tintShift = params.coatingStyle == 0
                                ? params.coatingWarmResponse
                                : (params.coatingStyle == 2 ? params.coatingCoolResponse : 1.0f);
    const int ghostCount =
        params.renderQuality == 0 ? std::min(params.ghostCount, 3) : (params.renderQuality == 2 ? params.ghostCount : std::min(params.ghostCount, 6));
    for (int i = 1; i <= ghostCount; ++i) {
      const float scale = 1.0f + params.ghostSpread * static_cast<float>(i);
      const float sx = cx - (x - cx) * scale * lensIdentityGhostScaleX(params);
      const float sy = cy - (y - cy) * scale * lensIdentityGhostScaleY(params);
      const Pixel ghost = mappedSample<T>(source, sx, sy, width, height, params);
      const float h = smoothstep(params.flareThreshold, 1.0f, luminance(ghost));
        const float depthBoost =
          1.0f + depthDefocusMaskAt(depth, sx, sy, params) * params.depthBloomBoost * 0.5f;
      const float w = h * (params.ghostIntensity / static_cast<float>(i)) * tintShift * depthBoost;
      add.r += ghost.r * params.ghostTint.r * w;
      add.g += ghost.g * params.ghostTint.g * w;
      add.b += ghost.b * params.ghostTint.b * w;
    }
  }

  const float centerGlow = smoothstep(params.flareThreshold * 0.9f, 1.0f, luminance(base));
  add.r += params.veil * centerGlow * params.centerVeilScale;
  add.g += params.veil * centerGlow * params.centerVeilScale;
  add.b += params.veil * centerGlow * params.centerVeilScale;

  return add;
}

template <typename T>
OfxStatus renderTyped(OfxImageEffectHandle instance, const Image &source, const Image &output,
                      const OfxRectI &renderWindow, const RenderParams &params,
                      const DepthImage *depth, const char *pixelTag) {
  const int width = source.bounds.x2 - source.bounds.x1;
  const int height = source.bounds.y2 - source.bounds.y1;
  const DebugView debugView = static_cast<DebugView>(params.debugView);
  const bool debugEnabled = debugView != DebugView::Off;
  const bool bypass = params.mix <= 0.0001f;
  const auto started = std::chrono::steady_clock::now();
  std::uint64_t pixelsWritten = 0;
  bool aborted = false;
  const bool hasDepth = depth && depth->image && depth->image->data;
  logPrintf(LogLevel::Debug,
            "render.cpu",
            "enter renderTyped pixelType=%s hasDepth=%d sourceData=%p sourceRowBytes=%d sourceBounds=[%d,%d,%d,%d] outputData=%p outputRowBytes=%d outputBounds=[%d,%d,%d,%d] depthData=%p depthRowBytes=%d depthBounds=[%d,%d,%d,%d]",
            pixelTag ? pixelTag : "unknown",
            hasDepth ? 1 : 0,
            source.data,
            source.rowBytes,
            source.bounds.x1,
            source.bounds.y1,
            source.bounds.x2,
            source.bounds.y2,
            output.data,
            output.rowBytes,
            output.bounds.x1,
            output.bounds.y1,
            output.bounds.x2,
            output.bounds.y2,
            hasDepth ? depth->image->data : nullptr,
            hasDepth ? depth->image->rowBytes : 0,
            hasDepth ? depth->image->bounds.x1 : 0,
            hasDepth ? depth->image->bounds.y1 : 0,
            hasDepth ? depth->image->bounds.x2 : 0,
            hasDepth ? depth->image->bounds.y2 : 0);

  for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
    if (y == renderWindow.y1) {
      logPrintf(LogLevel::Trace,
                "render.cpu",
                "row begin pixelType=%s y=%d/%d",
                pixelTag ? pixelTag : "unknown",
                y,
                renderWindow.y2 - 1);
    }
    if (gEffectSuite->abort(instance)) {
      aborted = true;
      logPrintf(LogLevel::Warn,
                "render.cpu",
                "abort requested pixelType=%s row=%d",
                pixelTag ? pixelTag : "unknown",
                y);
      return kOfxStatOK;
    }

    for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
      T *dst = pixelAddress<T>(output, x, y);
      if (!dst) {
        continue;
      }

      const Pixel original = sampleNearest<T>(source, static_cast<float>(x), static_cast<float>(y));

      if (debugEnabled) {
        Pixel debugColor = original;
        if (debugView == DebugView::Depth) {
          const float d = depth ? sampleDepthAt(depth, static_cast<float>(x), static_cast<float>(y), params) : 0.0f;
          debugColor = {d, d, d, 1.0f};
        } else if (debugView == DebugView::DepthFocusMask) {
          const float m = depth ? depthDefocusMaskAt(depth, static_cast<float>(x), static_cast<float>(y), params)
                                : 0.0f;
          debugColor = {m, m, m, 1.0f};
        } else if (debugView == DebugView::HighlightMatte) {
          const Pixel mapped = mappedSample<T>(source, static_cast<float>(x), static_cast<float>(y),
                                               width, height, params);
          const float h = highlightMatteAt(mapped, params);
          debugColor = {h, h, h, 1.0f};
        } else if (debugView == DebugView::EdgeMask) {
          const float e = edgeMaskAt(source, static_cast<float>(x), static_cast<float>(y), width, height,
                                     params, depth);
          debugColor = {e, e, e, 1.0f};
        }

        writePixelTyped(dst, debugColor);
        ++pixelsWritten;
        continue;
      }

      if (bypass) {
        writePixelTyped(dst, original);
        continue;
      }

      Pixel color = opticalBaseSample<T>(source, static_cast<float>(x), static_cast<float>(y), width, height,
                                         params, depth);
      color = edgeCharacter<T>(source, static_cast<float>(x), static_cast<float>(y), width, height, color,
                               params, depth);
      color = depthDefocusCharacter<T>(source, static_cast<float>(x), static_cast<float>(y), width, height,
                                       color, params, depth);
      const Pixel add = lensAdditives<T>(source, static_cast<float>(x), static_cast<float>(y), width, height,
                                         params, color, depth);
      color.r += add.r;
      color.g += add.g;
      color.b += add.b;
      color = applyVignetteAndGuides(color, static_cast<float>(x - source.bounds.x1),
                                     static_cast<float>(y - source.bounds.y1), width, height, params);
      color = composeFinalPixel(original, color, params);
      writePixelTyped(dst, color);
      ++pixelsWritten;
    }
    if (y == renderWindow.y1) {
      logPrintf(LogLevel::Trace,
                "render.cpu",
                "row end pixelType=%s y=%d pixelsWritten=%llu",
                pixelTag ? pixelTag : "unknown",
                y,
                static_cast<unsigned long long>(pixelsWritten));
    }
  }

  const auto elapsed = std::chrono::steady_clock::now() - started;
  const double elapsedMs =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count();
  const double elapsedSec = elapsedMs > 0.0 ? elapsedMs / 1000.0 : 0.0;
  const double megaPixelsPerSec = elapsedSec > 0.0 ?
      (static_cast<double>(pixelsWritten) / 1000000.0) / elapsedSec : 0.0;
  const LogLevel perfLevel = elapsedMs >= slowMsThreshold() ? LogLevel::Warn : LogLevel::Debug;
  logPrintf(perfLevel,
            "render.cpu",
            "renderTyped pixelType=%s window=[%d,%d,%d,%d] pixels=%llu elapsed=%.3fms throughput=%.2fMPix/s aborted=%d",
            pixelTag ? pixelTag : "unknown",
            renderWindow.x1,
            renderWindow.y1,
            renderWindow.x2,
            renderWindow.y2,
            static_cast<unsigned long long>(pixelsWritten),
            elapsedMs,
            megaPixelsPerSec,
            aborted ? 1 : 0);

  return kOfxStatOK;
}

template <typename T>
void fillEmptyTyped(const Image &output, const OfxRectI &renderWindow) {
  for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
    for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
      T *dst = pixelAddress<T>(output, x, y);
      if (dst) {
        writePixelTyped(dst, {});
      }
    }
  }
}

bool clipConnected(OfxImageClipHandle clip);

bool fetchImage(OfxImageClipHandle clip, OfxTime time, OfxPropertySetHandle *imageHandle, Image *image) {
  if (!clip || !imageHandle || !image) {
    return false;
  }

  *imageHandle = nullptr;
  *image = {};

  if (gEffectSuite->clipGetImage(clip, time, nullptr, imageHandle) != kOfxStatOK || !*imageHandle) {
    return false;
  }

  if (gPropertySuite->propGetPointer(*imageHandle, kOfxImagePropData, 0, &image->data) != kOfxStatOK ||
      gPropertySuite->propGetInt(*imageHandle, kOfxImagePropRowBytes, 0, &image->rowBytes) != kOfxStatOK ||
      gPropertySuite->propGetIntN(*imageHandle, kOfxImagePropBounds, 4, &image->bounds.x1) != kOfxStatOK) {
    gEffectSuite->clipReleaseImage(*imageHandle);
    *imageHandle = nullptr;
    *image = {};
    return false;
  }

  if (!image->data) {
    gEffectSuite->clipReleaseImage(*imageHandle);
    *imageHandle = nullptr;
    *image = {};
    return false;
  }

  return true;
}

bool fetchOptionalImage(OfxImageClipHandle clip, OfxTime time, OfxPropertySetHandle *imageHandle,
                        Image *image) {
  if (!imageHandle || !image || !gEffectSuite || !gPropertySuite) {
    return false;
  }

  *imageHandle = nullptr;
  *image = {};

  if (!clip || !clipConnected(clip)) {
    return false;
  }

  return fetchImage(clip, time, imageHandle, image);
}

bool getImageString(OfxPropertySetHandle imageHandle, const char *property, char **value) {
  return imageHandle && value &&
         gPropertySuite->propGetString(imageHandle, property, 0, value) == kOfxStatOK && *value != nullptr;
}

bool stringsMatch(const char *a, const char *b) {
  return a && b && std::strcmp(a, b) == 0;
}

std::string clipPropertyName(const char *prefix, const char *clipName) {
  std::string name(prefix);
  name += clipName;
  return name;
}

bool clipConnected(OfxImageClipHandle clip) {
  if (!clip) {
    return false;
  }

  OfxPropertySetHandle clipProps = nullptr;
  if (gEffectSuite->clipGetPropertySet(clip, &clipProps) != kOfxStatOK || !clipProps) {
    return false;
  }

  int connected = 0;
  return gPropertySuite->propGetInt(clipProps, kOfxImageClipPropConnected, 0, &connected) == kOfxStatOK &&
         connected != 0;
}

bool validateDepthGeometry(const Image &source, const Image &depth) {
  const int sourceWidth = source.bounds.x2 - source.bounds.x1;
  const int sourceHeight = source.bounds.y2 - source.bounds.y1;
  const int depthWidth = depth.bounds.x2 - depth.bounds.x1;
  const int depthHeight = depth.bounds.y2 - depth.bounds.y1;
  return sourceWidth == depthWidth && sourceHeight == depthHeight;
}

float roiPaddingPixels(const RenderParams &params) {
  const float flarePadding = params.flareIntensity > 0.001f
                                 ? params.flareLength * params.flareSpanScale * lensIdentityFlareScale(params) * 512.0f
                                 : 0.0f;
  const float bloomPadding = (params.veil > 0.001f || params.highlightCream > 0.001f)
                                 ? params.bloomRadius * params.bloomPixelScale * lensIdentityBloomScale(params)
                                 : 0.0f;
  const float edgePadding = std::max(params.edgeBlur * params.edgeBlurPixels,
                                     (params.tangentialSmear + params.horizontalSmear) * params.smearPixels);
  const float chromaticPadding = params.lateralCA * params.lateralCAPixelScale + params.longitudinalCA * 3.0f;
  const float depthPadding = params.depthMapEnabled != 0 ? params.depthDefocusPixels * params.depthInfluence : 0.0f;
  return std::max({flarePadding, bloomPadding, edgePadding, chromaticPadding, depthPadding, 2.0f});
}

} // namespace

OfxStatus render(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs) {
  ScopedLogTimer renderTimer(LogLevel::Info, "render", "render");
  renderTimer.setResult("in_progress");

  OfxTime time = 0.0;
  OfxRectI renderWindow{};
  const char *stage = "read_in_args";
  if (gPropertySuite->propGetDouble(inArgs, kOfxPropTime, 0, &time) != kOfxStatOK ||
      gPropertySuite->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1) != kOfxStatOK) {
    logMessage(LogLevel::Error, "render", "failed to read render time/window");
    renderTimer.setResult("failed_read_in_args");
    return kOfxStatFailed;
  }
  logPrintf(LogLevel::Debug,
            "render",
            "start time=%.3f window=[%d,%d,%d,%d]",
            time,
            renderWindow.x1,
            renderWindow.y1,
            renderWindow.x2,
            renderWindow.y2);

  OfxImageClipHandle sourceClip = nullptr;
  OfxImageClipHandle depthClip = nullptr;
  OfxImageClipHandle outputClip = nullptr;
  stage = "get_clips";
  if (gEffectSuite->clipGetHandle(instance, kOfxImageEffectSimpleSourceClipName, &sourceClip, nullptr) !=
          kOfxStatOK ||
      gEffectSuite->clipGetHandle(instance, kOfxImageEffectOutputClipName, &outputClip, nullptr) != kOfxStatOK ||
      !sourceClip || !outputClip) {
    logMessage(LogLevel::Error, "render", "failed to acquire source/output clips");
    renderTimer.setResult("failed_get_clips");
    return kOfxStatErrBadHandle;
  }
  gEffectSuite->clipGetHandle(instance, kDepthClipName, &depthClip, nullptr);
  logPrintf(LogLevel::Debug,
            "render",
            "clip handles source=%p output=%p depth=%p depthConnected=%d",
            sourceClip,
            outputClip,
            depthClip,
            clipConnected(depthClip) ? 1 : 0);

  OfxPropertySetHandle sourceImageHandle = nullptr;
  OfxPropertySetHandle depthImageHandle = nullptr;
  OfxPropertySetHandle outputImageHandle = nullptr;
  Image source{};
  Image depth{};
  Image output{};
  DepthImage depthImage{};
  OfxStatus status = kOfxStatOK;

  try {
    stage = "fetch_output";
    if (!fetchImage(outputClip, time, &outputImageHandle, &output)) {
      status = gEffectSuite->abort(instance) ? kOfxStatOK : kOfxStatFailed;
      logPrintf(LogLevel::Error,
                "render",
                "failed to fetch output image abort=%d status=%s",
                gEffectSuite->abort(instance) ? 1 : 0,
                ofxStatusToString(status));
    } else {
      char *outputBitDepth = nullptr;
      char *outputComponents = nullptr;
      stage = "validate_output";
      if (!getImageString(outputImageHandle, kOfxImageEffectPropPixelDepth, &outputBitDepth) ||
          !getImageString(outputImageHandle, kOfxImageEffectPropComponents, &outputComponents) ||
          !stringsMatch(outputComponents, kOfxImageComponentRGBA)) {
        status = kOfxStatErrUnsupported;
        logMessage(LogLevel::Error, "render", "unsupported output format/components");
      } else {
        stage = "fetch_source";
        const bool hasSource = fetchImage(sourceClip, time, &sourceImageHandle, &source);
        char *sourceBitDepth = nullptr;
        char *sourceComponents = nullptr;
        if (hasSource &&
            (!getImageString(sourceImageHandle, kOfxImageEffectPropPixelDepth, &sourceBitDepth) ||
             !getImageString(sourceImageHandle, kOfxImageEffectPropComponents, &sourceComponents) ||
             !stringsMatch(sourceBitDepth, outputBitDepth) ||
             !stringsMatch(sourceComponents, kOfxImageComponentRGBA))) {
          status = kOfxStatErrUnsupported;
          logMessage(LogLevel::Error, "render", "source/output clip format mismatch");
        } else if (!hasSource) {
          logMessage(LogLevel::Warn,
                     "render",
                     "source image unavailable, returning transparent output for this render");
          if (std::strcmp(outputBitDepth, kOfxBitDepthByte) == 0) {
            fillEmptyTyped<OfxRGBAColourB>(output, renderWindow);
          } else if (std::strcmp(outputBitDepth, kOfxBitDepthShort) == 0) {
            fillEmptyTyped<OfxRGBAColourS>(output, renderWindow);
          } else if (std::strcmp(outputBitDepth, kOfxBitDepthFloat) == 0) {
            fillEmptyTyped<OfxRGBAColourF>(output, renderWindow);
          } else {
            status = kOfxStatErrUnsupported;
          }
        } else {
          const int sourceBpp = (std::strcmp(outputBitDepth, kOfxBitDepthByte) == 0)
                                    ? static_cast<int>(sizeof(OfxRGBAColourB))
                                    : (std::strcmp(outputBitDepth, kOfxBitDepthShort) == 0)
                                          ? static_cast<int>(sizeof(OfxRGBAColourS))
                                          : static_cast<int>(sizeof(OfxRGBAColourF));
          const bool sourceLayoutValid = hasSource ? validateImageLayout(source, sourceBpp) : true;
          const bool outputLayoutValid = validateImageLayout(output, sourceBpp);
          if (!sourceLayoutValid || !outputLayoutValid) {
            if (hasSource) {
              logImageLayout(LogLevel::Error, "render", "source", source, sourceBpp, sourceLayoutValid);
            }
            logImageLayout(LogLevel::Error, "render", "output", output, sourceBpp, outputLayoutValid);
            status = kOfxStatFailed;
            logMessage(LogLevel::Error, "render", "invalid source/output image layout");
          }

          if (status == kOfxStatOK) {
            stage = "read_params";
            RenderParams params = readParams(instance);
            logPrintf(LogLevel::Debug,
                      "render",
                      "params mix=%.3f quality=%d depthEnabled=%d lensIdentity=%d",
                      params.mix,
                      params.renderQuality,
                      params.depthMapEnabled,
                      params.lensIdentity);

            stage = "fetch_depth";
            const bool depthRequested = params.depthMapEnabled != 0;
            const bool depthClipAvailable = depthClip != nullptr;
            const bool depthClipIsConnected = clipConnected(depthClip);
            const bool depthImageFetched = depthRequested &&
                                           fetchOptionalImage(depthClip, time, &depthImageHandle, &depth);
            bool hasDepth = depthImageFetched;
            logPrintf(LogLevel::Debug,
                  "render",
                  "depth preflight requested=%d clipAvailable=%d clipConnected=%d fetched=%d handle=%p data=%p rowBytes=%d bounds=[%d,%d,%d,%d]",
                  depthRequested ? 1 : 0,
                  depthClipAvailable ? 1 : 0,
                  depthClipIsConnected ? 1 : 0,
                  depthImageFetched ? 1 : 0,
                  depthImageHandle,
              depthImageFetched ? depth.data : nullptr,
              depthImageFetched ? depth.rowBytes : 0,
              depthImageFetched ? depth.bounds.x1 : 0,
              depthImageFetched ? depth.bounds.y1 : 0,
              depthImageFetched ? depth.bounds.x2 : 0,
              depthImageFetched ? depth.bounds.y2 : 0);
            char *depthBitDepth = nullptr;
            char *depthComponents = nullptr;
            if (hasDepth) {
              if (!getImageString(depthImageHandle, kOfxImageEffectPropPixelDepth, &depthBitDepth) ||
                  !getImageString(depthImageHandle, kOfxImageEffectPropComponents, &depthComponents) ||
                  !stringsMatch(depthBitDepth, outputBitDepth) ||
                  !stringsMatch(depthComponents, kOfxImageComponentRGBA) ||
                  !parseDepthBitDepth(depthBitDepth, &depthImage.bitDepth) ||
                  !parseDepthComponents(depthComponents, &depthImage.components)) {
                hasDepth = false;
                logMessage(LogLevel::Warn,
                           "render",
                           "depth clip present but format incompatible with output, depth disabled");
              } else {
                const int depthBpp = bytesPerChannel(depthImage.bitDepth) *
                                     (depthImage.components == DepthComponents::RGBA ? 4 : 1);
                const bool depthLayoutValid = validateImageLayout(depth, depthBpp);
                const bool depthGeometryValid = hasSource ? validateDepthGeometry(source, depth) : true;
                const bool depthRowDirectionValid = depth.rowBytes > 0;
                if (!depthLayoutValid || !depthGeometryValid || !depthRowDirectionValid) {
                  logImageLayout(LogLevel::Warn, "render", "depth", depth, depthBpp, false);
                  if (!depthGeometryValid && hasSource) {
                    logPrintf(LogLevel::Warn,
                              "render",
                              "depth clip disabled due to geometry mismatch source=[%d,%d,%d,%d] depth=[%d,%d,%d,%d]",
                              source.bounds.x1,
                              source.bounds.y1,
                              source.bounds.x2,
                              source.bounds.y2,
                              depth.bounds.x1,
                              depth.bounds.y1,
                              depth.bounds.x2,
                              depth.bounds.y2);
                  }
                  if (!depthRowDirectionValid) {
                    logPrintf(LogLevel::Warn,
                              "render",
                              "depth clip disabled due to unsupported row direction rowBytes=%d",
                              depth.rowBytes);
                  }
                  logPrintf(LogLevel::Warn,
                            "render",
                            "depth clip disabled due to invalid layout/format bitDepth=%s components=%s",
                            depthBitDepth ? depthBitDepth : "(null)",
                            depthComponents ? depthComponents : "(null)");
                  hasDepth = false;
                } else {
                  logImageLayout(LogLevel::Debug, "render", "depth", depth, depthBpp, true);
                  logPrintf(LogLevel::Debug,
                            "render",
                            "depth accepted bitDepth=%s components=%s bytesPerPixel=%d",
                            depthBitDepth ? depthBitDepth : "(null)",
                            depthComponents ? depthComponents : "(null)",
                            depthBpp);
                  depthImage.image = &depth;
                }
              }
            }
            {
              stage = "dispatch_render";
              if (hasSource) {
                params.edgeCropScale = automaticEdgeCropScale(source, source.bounds.x2 - source.bounds.x1,
                                                             source.bounds.y2 - source.bounds.y1, params);
              }
              const bool depthDebugView = params.debugView == static_cast<int>(DebugView::Depth);
              const bool depthUsedForProcessing = hasDepth && depthDebugView;
              const bool useMetal = false;
              logPrintf(LogLevel::Info,
                        "render",
                        "render path source=%d depth=%d depthUsed=%d depthConnected=%d metal=%d outputBitDepth=%s",
                        hasSource ? 1 : 0,
                        hasDepth ? 1 : 0,
                        depthUsedForProcessing ? 1 : 0,
                        depthClipIsConnected ? 1 : 0,
                        useMetal ? 1 : 0,
                        outputBitDepth ? outputBitDepth : "(null)");
              if (std::strcmp(outputBitDepth, kOfxBitDepthByte) == 0) {
                if (hasSource) {
                  logPrintf(LogLevel::Debug,
                            "render",
                            "dispatch cpu pixelType=byte hasDepth=%d sourceData=%p sourceRowBytes=%d outputData=%p outputRowBytes=%d depthData=%p depthRowBytes=%d",
                            hasDepth ? 1 : 0,
                            source.data,
                            source.rowBytes,
                            output.data,
                            output.rowBytes,
                            depthUsedForProcessing ? depth.data : nullptr,
                            depthUsedForProcessing ? depth.rowBytes : 0);
                  status = renderTyped<OfxRGBAColourB>(instance, source, output, renderWindow, params,
                                                       depthUsedForProcessing ? &depthImage : nullptr,
                                                       "byte");
                } else {
                  fillEmptyTyped<OfxRGBAColourB>(output, renderWindow);
                }
              } else if (std::strcmp(outputBitDepth, kOfxBitDepthShort) == 0) {
                if (hasSource) {
                  logPrintf(LogLevel::Debug,
                            "render",
                            "dispatch cpu pixelType=short hasDepth=%d sourceData=%p sourceRowBytes=%d outputData=%p outputRowBytes=%d depthData=%p depthRowBytes=%d",
                            hasDepth ? 1 : 0,
                            source.data,
                            source.rowBytes,
                            output.data,
                            output.rowBytes,
                            depthUsedForProcessing ? depth.data : nullptr,
                            depthUsedForProcessing ? depth.rowBytes : 0);
                  status = renderTyped<OfxRGBAColourS>(instance, source, output, renderWindow, params,
                                                       depthUsedForProcessing ? &depthImage : nullptr,
                                                       "short");
                } else {
                  fillEmptyTyped<OfxRGBAColourS>(output, renderWindow);
                }
              } else if (std::strcmp(outputBitDepth, kOfxBitDepthFloat) == 0) {
                if (hasSource) {
                  logPrintf(LogLevel::Debug,
                            "render",
                            "dispatch cpu pixelType=float hasDepth=%d sourceData=%p sourceRowBytes=%d outputData=%p outputRowBytes=%d depthData=%p depthRowBytes=%d",
                            hasDepth ? 1 : 0,
                            source.data,
                            source.rowBytes,
                            output.data,
                            output.rowBytes,
                            depthUsedForProcessing ? depth.data : nullptr,
                            depthUsedForProcessing ? depth.rowBytes : 0);
                  status = renderTyped<OfxRGBAColourF>(instance, source, output, renderWindow, params,
                                                       depthUsedForProcessing ? &depthImage : nullptr,
                                                       "float");
                } else {
                  fillEmptyTyped<OfxRGBAColourF>(output, renderWindow);
                }
              } else {
                status = kOfxStatErrUnsupported;
                logMessage(LogLevel::Error, "render", "unsupported output bit depth");
              }
            }
          }
        }
      }
    }
  } catch (const std::exception &ex) {
    status = kOfxStatErrUnknown;
    logPrintf(LogLevel::Error,
              "render",
              "exception stage=%s what=%s",
              stage,
              ex.what());
  } catch (...) {
    status = kOfxStatErrUnknown;
    logPrintf(LogLevel::Error, "render", "unknown exception stage=%s", stage);
  }

  if (sourceImageHandle) {
    gEffectSuite->clipReleaseImage(sourceImageHandle);
  }
  if (depthImageHandle) {
    gEffectSuite->clipReleaseImage(depthImageHandle);
  }
  if (outputImageHandle) {
    gEffectSuite->clipReleaseImage(outputImageHandle);
  }

  renderTimer.setResult(ofxStatusToString(status));
  logPrintf(LogLevel::Info, "render", "end status=%s (%d)", ofxStatusToString(status), status);
  return status;
}

OfxStatus isIdentity(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs,
                     OfxPropertySetHandle outArgs) {
  if (!outArgs) {
    return kOfxStatReplyDefault;
  }

  const RenderParams params = readParams(instance);
  if (params.mix > 0.0001f) {
    return kOfxStatReplyDefault;
  }

  OfxTime time = 0.0;
  gPropertySuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
  gPropertySuite->propSetString(outArgs, kOfxPropName, 0, kOfxImageEffectSimpleSourceClipName);
  gPropertySuite->propSetDouble(outArgs, kOfxPropTime, 0, time);
  return kOfxStatOK;
}

OfxStatus getRegionOfDefinition(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs,
                                OfxPropertySetHandle outArgs) {
  if (!outArgs) {
    return kOfxStatReplyDefault;
  }

  OfxImageClipHandle sourceClip = nullptr;
  if (gEffectSuite->clipGetHandle(instance, kOfxImageEffectSimpleSourceClipName, &sourceClip, nullptr) !=
          kOfxStatOK ||
      !sourceClip) {
    return kOfxStatReplyDefault;
  }

  OfxTime time = 0.0;
  gPropertySuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
  OfxRectD sourceRod{};
  if (gEffectSuite->clipGetRegionOfDefinition(sourceClip, time, &sourceRod) != kOfxStatOK) {
    return kOfxStatReplyDefault;
  }

  gPropertySuite->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, &sourceRod.x1);
  return kOfxStatOK;
}

OfxStatus getRegionsOfInterest(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs,
                               OfxPropertySetHandle outArgs) {
  if (!outArgs) {
    return kOfxStatReplyDefault;
  }

  double roi[4] = {};
  if (gPropertySuite->propGetDoubleN(inArgs, kOfxImageEffectPropRegionOfInterest, 4, roi) != kOfxStatOK) {
    return kOfxStatReplyDefault;
  }

  const RenderParams params = readParams(instance);
  const double padding = static_cast<double>(roiPaddingPixels(params));
  roi[0] -= padding;
  roi[1] -= padding;
  roi[2] += padding;
  roi[3] += padding;

  const std::string sourceRoi =
      clipPropertyName("OfxImageClipPropRoI_", kOfxImageEffectSimpleSourceClipName);
  const std::string depthRoi = clipPropertyName("OfxImageClipPropRoI_", kDepthClipName);
  gPropertySuite->propSetDoubleN(outArgs, sourceRoi.c_str(), 4, roi);
  gPropertySuite->propSetDoubleN(outArgs, depthRoi.c_str(), 4, roi);
  return kOfxStatOK;
}

OfxStatus getClipPreferences(OfxImageEffectHandle /*instance*/, OfxPropertySetHandle outArgs) {
  if (!outArgs) {
    return kOfxStatReplyDefault;
  }

  const std::string outputComponents =
      clipPropertyName("OfxImageClipPropComponents_", kOfxImageEffectOutputClipName);
  const std::string sourceComponents =
      clipPropertyName("OfxImageClipPropComponents_", kOfxImageEffectSimpleSourceClipName);
    const std::string depthComponents = clipPropertyName("OfxImageClipPropComponents_", kDepthClipName);
  gPropertySuite->propSetString(outArgs, outputComponents.c_str(), 0, kOfxImageComponentRGBA);
  gPropertySuite->propSetString(outArgs, sourceComponents.c_str(), 0, kOfxImageComponentRGBA);
    gPropertySuite->propSetString(outArgs, depthComponents.c_str(), 0, kOfxImageComponentRGBA);
  gPropertySuite->propSetString(outArgs, kOfxImageEffectPropPreMultiplication, 0,
                                kOfxImageUnPreMultiplied);
  gPropertySuite->propSetInt(outArgs, kOfxImageClipPropContinuousSamples, 0, 0);
  gPropertySuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 0);
  return kOfxStatOK;
}

} // namespace rimell
