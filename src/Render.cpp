#include "Render.h"

#include "Constants.h"
#include "Diagnostics.h"
#include "HostSuites.h"
#include "LensMap.h"
#include "MetalRender.h"
#include "Parameters.h"
#include "PixelAccess.h"
#include "RenderCore.h"

#include "ofxGPURender.h"
#include "ofxPixels.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <string>
#include <thread>
#include <vector>

namespace rimell {
namespace {

enum class DebugView {
  Off = 0,
  Source = 1,
  HighlightMatte = 2,
  EdgeMask = 3,
  MetalIdentity = 4,
  MetalBilinear = 5,
  MetalBasicGeometry = 6,
};

constexpr unsigned int kMaxCpuRenderWorkers = 8;

struct InstanceData {};

const char *imageStorageName(ImageStorage storage) {
  switch (storage) {
  case ImageStorage::Cpu:
    return "cpu";
  case ImageStorage::Metal:
    return "metal";
  }
  return "unknown";
}

void readIntProperty(OfxPropertySetHandle properties, const char *name, int *value) {
  if (!properties || !name || !value || !gPropertySuite) {
    return;
  }
  (void)gPropertySuite->propGetInt(properties, name, 0, value);
}

void readPointerProperty(OfxPropertySetHandle properties, const char *name, void **value) {
  if (!properties || !name || !value || !gPropertySuite) {
    return;
  }
  (void)gPropertySuite->propGetPointer(properties, name, 0, value);
}

OfxTime readTimeProperty(OfxPropertySetHandle properties) {
  OfxTime time = 0.0;
  if (!properties || !gPropertySuite) {
    return time;
  }
  (void)gPropertySuite->propGetDouble(properties, kOfxPropTime, 0, &time);
  return time;
}

bool metalRuntimeAvailable() {
#if defined(__APPLE__)
  return true;
#else
  return false;
#endif
}

bool isMetalDiagnosticDebugView(int debugView) {
  return debugView == static_cast<int>(DebugView::MetalIdentity) ||
         debugView == static_cast<int>(DebugView::MetalBilinear) ||
         debugView == static_cast<int>(DebugView::MetalBasicGeometry);
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

bool validateCpuImageLayout(const Image &image, int bytesPerPixel) {
  if (image.storage != ImageStorage::Cpu) {
    return false;
  }
  const int width = image.bounds.x2 - image.bounds.x1;
  const int height = image.bounds.y2 - image.bounds.y1;
  if (!image.data || image.rowBytes == 0 || width <= 0 || height <= 0 || bytesPerPixel <= 0) {
    return false;
  }

  const int absRowBytes = image.rowBytes < 0 ? -image.rowBytes : image.rowBytes;
  const long long required = static_cast<long long>(width) * static_cast<long long>(bytesPerPixel);
  return static_cast<long long>(absRowBytes) >= required;
}

bool validateMetalImageLayout(const Image &image) {
  if (image.storage != ImageStorage::Metal) {
    return false;
  }

  const int width = image.bounds.x2 - image.bounds.x1;
  const int height = image.bounds.y2 - image.bounds.y1;
  return image.data && image.rowBytes > 0 && width > 0 && height > 0;
}

void logImageLayout(LogLevel level, const char *scope, const char *name, const Image &image,
                    int bytesPerPixel, bool valid) {
  const int width = image.bounds.x2 - image.bounds.x1;
  const int height = image.bounds.y2 - image.bounds.y1;
  const int absRowBytes = image.rowBytes < 0 ? -image.rowBytes : image.rowBytes;
  const long long required = static_cast<long long>(width) * static_cast<long long>(bytesPerPixel);
  logPrintf(level,
            scope,
            "%s layout storage=%s valid=%d data=%p bounds=[%d,%d,%d,%d] size=%dx%d rowBytes=%d absRowBytes=%d bpp=%d minRowBytes=%lld",
            name ? name : "image",
            imageStorageName(image.storage),
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

float highlightMatteAt(const Pixel &sample, const RenderParams &params) {
  return clamp01(smoothstep(params.flareThreshold, 1.0f, luminance(sample)));
}

float edgeMaskAt(const Image &source, float x, float y, int width, int height,
                 const RenderParams &params) {
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
  const LensMap map = buildLensMap(x, y, source, width, height, params);
  const float radius = std::max(map.radius, std::sqrt(nx * nx + ny * ny));
  const float edge = std::max({map.edgeMask,
                               smoothstep(std::max(0.0f, 1.0f - params.radialFalloff), 1.15f, radius)});
  return clamp01(edge);
}

float qualityScale(const RenderParams &params) {
  switch (params.renderQuality) {
  case 0:
    return 0.35f;
  case 1:
    return 0.65f;
  case 2:
    return 1.0f;
  case 3:
    return 1.5f;
  default:
    return 1.0f;
  }
}

int cappedEdgeBlurSamples(const RenderParams &params) {
  switch (params.renderQuality) {
  case 0:
    return 1;
  case 1:
    return 2;
  case 3:
    return 6;
  default:
    return 4;
  }
}

int cappedSmearSamples(const RenderParams &params) {
  switch (params.renderQuality) {
  case 0:
    return 1;
  case 1:
    return 2;
  case 3:
    return 5;
  default:
    return 3;
  }
}

int cappedFlareSteps(const RenderParams &params) {
  const int requested =
      std::max(1, static_cast<int>(std::round((2.0f + params.flareLength * params.flareStepDensity) *
                                              qualityScale(params))));
  const int cap = params.renderQuality == 0 ? 8 : (params.renderQuality == 1 ? 16 : (params.renderQuality == 3 ? 48 : 32));
  return std::min(requested, cap);
}

int cappedBloomRings(const RenderParams &params) {
  const int requested =
      std::max(1, static_cast<int>(std::round(static_cast<float>(params.bloomRings) * qualityScale(params))));
  const int cap = params.renderQuality == 0 ? 1 : (params.renderQuality == 1 ? 2 : (params.renderQuality == 3 ? 8 : 3));
  return std::min(requested, cap);
}

int cappedBloomSamplesPerRing(const RenderParams &params) {
  const int requested = std::max(
      3, static_cast<int>(std::round(static_cast<float>(params.bloomSamplesPerRing) * qualityScale(params))));
  const int cap = params.renderQuality == 0 ? 4 : (params.renderQuality == 1 ? 6 : (params.renderQuality == 3 ? 16 : 8));
  return std::min(requested, cap);
}

int cappedGhostCount(const RenderParams &params) {
  const int cap = params.renderQuality == 0 ? 0 : (params.renderQuality == 1 ? 1 : (params.renderQuality == 3 ? 8 : 4));
  return std::min(params.ghostCount, cap);
}

float cappedCAPixels(const RenderParams &params) {
  const float cap = params.renderQuality == 0 ? 1.0f : (params.renderQuality == 1 ? 2.0f : (params.renderQuality == 3 ? 8.0f : 4.0f));
  return std::min(params.lateralCA * params.lateralCAPixelScale, cap);
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
                        const RenderParams &params) {
  const float caPixels = params.lateralCA * params.lateralCAPixelScale;
  Pixel base = warpedSourceSample<T>(source, x, y, width, height, params, 0.0f);
  if (caPixels > 0.01f) {
    const float cappedCA = cappedCAPixels(params);
    Pixel red = warpedSourceSample<T>(source, x, y, width, height, params, cappedCA);
    Pixel blue = warpedSourceSample<T>(source, x, y, width, height, params, -cappedCA);
    base.r = red.r;
    base.b = blue.b;
  }

  if (params.longitudinalCA > 0.001f) {
    const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
    const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
    const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
    const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
    const float edge = smoothstep(0.15f, 1.05f, std::sqrt(nx * nx + ny * ny));
    const float focusBias = 0.35f + std::abs(params.focusDistance - 0.5f) * 1.3f;
    const float radiusPixels = params.longitudinalCA * focusBias * edge * 4.0f;
    const Pixel redDefocus = channelDefocusSample<T>(source, x, y, width, height, params, radiusPixels);
    const Pixel blueDefocus = channelDefocusSample<T>(source, x, y, width, height, params, radiusPixels * 0.65f);
    const float amount = clamp01(params.longitudinalCA * (0.35f + edge * 0.65f));
    base.r = lerp(base.r, redDefocus.r, amount);
    base.b = lerp(base.b, blueDefocus.b, amount);
  }

  return base;
}

template <typename T>
Pixel edgeCharacter(const Image &source, float x, float y, int width, int height, const Pixel &base,
                    const RenderParams &params) {
  if (params.edgeBlur <= 0.001f && params.fieldCurvature <= 0.001f &&
      params.tangentialSmear <= 0.001f && params.horizontalSmear <= 0.001f &&
      params.verticalSharpness <= 0.001f) {
    return base;
  }

  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
  const LensMap map = buildLensMap(x, y, source, width, height, params);
  const float radius = std::max(map.radius, std::sqrt(nx * nx + ny * ny));
  const float edge = std::max({map.edgeMask,
                               smoothstep(std::max(0.0f, 1.0f - params.radialFalloff), 1.15f, radius)});

  Pixel result = base;

  const float blurRadius =
      params.edgeBlur * edge * params.edgeBlurPixels + params.fieldCurvature * edge * params.fieldCurvaturePixels;
  if (blurRadius > 0.05f) {
    Pixel blur{};
    float weight = 0.0f;
    const int blurSamples = cappedEdgeBlurSamples(params);
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
    const int smearSamples = cappedSmearSamples(params);
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
                    const Pixel &base) {
  Pixel add{};

  const bool flareEnabled = params.flareIntensity > 0.001f && params.flareLength > 0.001f;
  const bool bloomEnabled = (params.veil > 0.001f || params.highlightCream > 0.001f) &&
                            params.bloomRadius > 0.001f && params.bloomPixelScale > 0.001f;
  const bool ghostEnabled = params.ghostIntensity > 0.001f && params.ghostCount > 0 && params.ghostSpread > 0.001f;
  const bool centerVeilEnabled = params.veil > 0.001f && params.centerVeilScale > 0.001f;
  if (!flareEnabled && !bloomEnabled && !ghostEnabled && !centerVeilEnabled) {
    return add;
  }

  const int flareSteps = cappedFlareSteps(params);
  const float flareSpan = params.flareLength * static_cast<float>(width) * params.flareSpanScale *
                          lensIdentityFlareScale(params);
  if (flareEnabled && flareSpan > 1.0f) {
    const float flareAngle = params.flareAngle * kPi / 180.0f;
    const float dirX = std::cos(flareAngle);
    const float dirY = std::sin(flareAngle);
    for (int i = -flareSteps; i <= flareSteps; ++i) {
      if (i == 0) {
        continue;
      }
      const float t = static_cast<float>(i) / static_cast<float>(flareSteps);
      const float sx = x + dirX * t * flareSpan;
      const float sy = y + dirY * t * flareSpan;
      const float h = highlightAt<T>(source, sx, sy, width, height, params, params.flareThreshold);
      const float w = std::exp(-std::abs(t) * params.flareFalloff) * h * params.flareIntensity;
      add.r += params.flareColour.r * w;
      add.g += params.flareColour.g * w;
      add.b += params.flareColour.b * w;
    }
  }

  const float bloomPixels = params.bloomRadius * params.bloomPixelScale;
  if (bloomEnabled && bloomPixels > 0.5f) {
    const float rotation = params.bokehRotation * kPi / 180.0f;
    const float cosR = std::cos(rotation);
    const float sinR = std::sin(rotation);
    const float stretch = (1.0f + params.bokehStretch * params.bokehStretchScale) *
                          lensIdentityBloomScale(params);
    const int rings = cappedBloomRings(params);
    const int samplesPerRing = cappedBloomSamplesPerRing(params);
    float total = 0.0f;
    Pixel bloom{};
    for (int ring = 1; ring <= rings; ++ring) {
      const float ringRadius = bloomPixels * static_cast<float>(ring) / static_cast<float>(rings);
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
          bokehEdgeKeep;
      add.r += bloom.r * amount;
      add.g += bloom.g * amount;
      add.b += bloom.b * amount;
    }
  }

  if (ghostEnabled) {
    const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
    const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
    const float tintShift = params.coatingStyle == 0
                                ? params.coatingWarmResponse
                                : (params.coatingStyle == 2 ? params.coatingCoolResponse : 1.0f);
    const int ghostCount = cappedGhostCount(params);
    for (int i = 1; i <= ghostCount; ++i) {
      const float scale = 1.0f + params.ghostSpread * static_cast<float>(i);
      const float sx = cx - (x - cx) * scale * lensIdentityGhostScaleX(params);
      const float sy = cy - (y - cy) * scale * lensIdentityGhostScaleY(params);
      const Pixel ghost = mappedSample<T>(source, sx, sy, width, height, params);
      const float h = smoothstep(params.flareThreshold, 1.0f, luminance(ghost));
      const float w = h * (params.ghostIntensity / static_cast<float>(i)) * tintShift;
      add.r += ghost.r * params.ghostTint.r * w;
      add.g += ghost.g * params.ghostTint.g * w;
      add.b += ghost.b * params.ghostTint.b * w;
    }
  }

  if (centerVeilEnabled) {
    const float centerGlow = smoothstep(params.flareThreshold * 0.9f, 1.0f, luminance(base));
    add.r += params.veil * centerGlow * params.centerVeilScale;
    add.g += params.veil * centerGlow * params.centerVeilScale;
    add.b += params.veil * centerGlow * params.centerVeilScale;
  }

  return add;
}

template <typename T>
OfxStatus renderTyped(OfxImageEffectHandle instance, const Image &source, const Image &output,
                      const OfxRectI &renderWindow, const RenderParams &params,
                      const char *pixelTag) {
  const int width = source.bounds.x2 - source.bounds.x1;
  const int height = source.bounds.y2 - source.bounds.y1;
  const DebugView debugView = static_cast<DebugView>(params.debugView);
  const bool debugEnabled = debugView != DebugView::Off;
  const bool bypass = params.mix <= 0.0001f;
  const auto started = std::chrono::steady_clock::now();
  std::atomic<std::uint64_t> pixelsWritten{0};
  std::atomic<bool> aborted{false};
  const int renderWidth = renderWindow.x2 - renderWindow.x1;
  const int renderHeight = renderWindow.y2 - renderWindow.y1;
  const unsigned int availableWorkers =
      std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 1u;
  const bool enableWorkerPool = !debugEnabled && renderWidth >= 1024 && renderHeight >= 720;
  const unsigned int workerCount = enableWorkerPool
                                       ? std::max(1u,
                                                  std::min<unsigned int>({availableWorkers, kMaxCpuRenderWorkers,
                                                                          static_cast<unsigned int>(std::max(1, renderHeight))}))
                                       : 1u;
  logPrintf(LogLevel::Debug,
            "render.cpu",
            "enter renderTyped pixelType=%s workers=%u workerPool=%d sourceData=%p sourceRowBytes=%d sourceBounds=[%d,%d,%d,%d] outputData=%p outputRowBytes=%d outputBounds=[%d,%d,%d,%d]",
            pixelTag ? pixelTag : "unknown",
            workerCount,
            enableWorkerPool ? 1 : 0,
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
            output.bounds.y2);

  auto renderRows = [&](int yBegin, int yEnd, unsigned int workerIndex) {
    std::uint64_t localPixels = 0;
    for (int y = yBegin; y < yEnd && !aborted.load(std::memory_order_relaxed); ++y) {
      if (workerIndex == 0 && y == yBegin) {
        logPrintf(LogLevel::Trace,
                  "render.cpu",
                  "row begin pixelType=%s y=%d/%d",
                  pixelTag ? pixelTag : "unknown",
                  y,
                  renderWindow.y2 - 1);
      }
      if (gEffectSuite->abort(instance)) {
        aborted.store(true, std::memory_order_relaxed);
        logPrintf(LogLevel::Warn,
                  "render.cpu",
                  "abort requested pixelType=%s row=%d",
                  pixelTag ? pixelTag : "unknown",
                  y);
        break;
      }

      for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
        T *dst = pixelAddress<T>(output, x, y);
        if (!dst) {
          continue;
        }

        const Pixel original = sampleNearest<T>(source, static_cast<float>(x), static_cast<float>(y));

        if (debugEnabled) {
          Pixel debugColor = original;
          if (debugView == DebugView::Source) {
            debugColor = original;
          } else if (debugView == DebugView::HighlightMatte) {
            const Pixel mapped =
                mappedSample<T>(source, static_cast<float>(x), static_cast<float>(y), width, height, params);
            const float h = highlightMatteAt(mapped, params);
            debugColor = {h, h, h, 1.0f};
          } else if (debugView == DebugView::EdgeMask) {
            const float e =
                edgeMaskAt(source, static_cast<float>(x), static_cast<float>(y), width, height, params);
            debugColor = {e, e, e, 1.0f};
          }

          writePixelTyped(dst, debugColor);
          ++localPixels;
          continue;
        }

        if (bypass) {
          writePixelTyped(dst, original);
          ++localPixels;
          continue;
        }

        Pixel color =
            opticalBaseSample<T>(source, static_cast<float>(x), static_cast<float>(y), width, height, params);
        color = edgeCharacter<T>(source, static_cast<float>(x), static_cast<float>(y), width, height, color, params);
        const Pixel add =
            lensAdditives<T>(source, static_cast<float>(x), static_cast<float>(y), width, height, params, color);
        color.r += add.r;
        color.g += add.g;
        color.b += add.b;
        color = applyVignetteAndGuides(color,
                                       static_cast<float>(x - source.bounds.x1),
                                       static_cast<float>(y - source.bounds.y1),
                                       width,
                                       height,
                                       params);
        color = composeFinalPixel(original, color, params);
        writePixelTyped(dst, color);
        ++localPixels;
      }
      if (workerIndex == 0 && y == yBegin) {
        logPrintf(LogLevel::Trace,
                  "render.cpu",
                  "row end pixelType=%s y=%d pixelsWritten=%llu",
                  pixelTag ? pixelTag : "unknown",
                  y,
                  static_cast<unsigned long long>(localPixels));
      }
    }
    pixelsWritten.fetch_add(localPixels, std::memory_order_relaxed);
  };

  if (workerCount == 1) {
    renderRows(renderWindow.y1, renderWindow.y2, 0);
  } else {
    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (unsigned int i = 0; i < workerCount; ++i) {
      const int yBegin =
          renderWindow.y1 + static_cast<int>((static_cast<long long>(renderHeight) * i) / workerCount);
      const int yEnd =
          renderWindow.y1 + static_cast<int>((static_cast<long long>(renderHeight) * (i + 1)) / workerCount);
      workers.emplace_back(renderRows, yBegin, yEnd, i);
    }
    for (std::thread &worker : workers) {
      worker.join();
    }
  }

  const auto elapsed = std::chrono::steady_clock::now() - started;
  const double elapsedMs =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count();
  const double elapsedSec = elapsedMs > 0.0 ? elapsedMs / 1000.0 : 0.0;
  const std::uint64_t totalPixelsWritten = pixelsWritten.load(std::memory_order_relaxed);
  const double megaPixelsPerSec = elapsedSec > 0.0 ?
      (static_cast<double>(totalPixelsWritten) / 1000000.0) / elapsedSec : 0.0;
  const LogLevel perfLevel = elapsedMs >= slowMsThreshold() ? LogLevel::Warn : LogLevel::Debug;
  logPrintf(perfLevel,
            "render.cpu",
            "renderTyped pixelType=%s workers=%u window=[%d,%d,%d,%d] pixels=%llu elapsed=%.3fms throughput=%.2fMPix/s aborted=%d",
            pixelTag ? pixelTag : "unknown",
            workerCount,
            renderWindow.x1,
            renderWindow.y1,
            renderWindow.x2,
            renderWindow.y2,
            static_cast<unsigned long long>(totalPixelsWritten),
            elapsedMs,
            megaPixelsPerSec,
            aborted.load(std::memory_order_relaxed) ? 1 : 0);

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

bool fetchImage(OfxImageClipHandle clip, OfxTime time, ImageStorage storage,
                OfxPropertySetHandle *imageHandle, Image *image) {
  if (!clip || !imageHandle || !image) {
    return false;
  }

  *imageHandle = nullptr;
  *image = {};
  image->storage = storage;

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

  image->storage = storage;

  return true;
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

float roiPaddingPixels(const RenderParams &params, float referenceExtent) {
  const float extent = std::max(1.0f, referenceExtent);
  const float flarePadding = params.flareIntensity > 0.001f
                                 ? params.flareLength * params.flareSpanScale * lensIdentityFlareScale(params) * extent
                                 : 0.0f;
  const float bloomPadding = (params.veil > 0.001f || params.highlightCream > 0.001f)
                                 ? params.bloomRadius * params.bloomPixelScale * lensIdentityBloomScale(params)
                                 : 0.0f;
  const float edgePadding = std::max(params.edgeBlur * params.edgeBlurPixels,
                                     (params.tangentialSmear + params.horizontalSmear) * params.smearPixels);
  const float chromaticPadding = params.lateralCA * params.lateralCAPixelScale + params.longitudinalCA * 3.0f;
  return std::max({flarePadding, bloomPadding, edgePadding, chromaticPadding, 2.0f});
}

bool canUseMetalImagesForThisRender(const RenderParams &params, bool hostMetal, bool hasSource,
                                    const Image &source, const Image &output, const char *sourceBitDepth,
                                    const char *sourceComponents, const char *outputBitDepth,
                                    const char *outputComponents, void *commandQueue) {
  if (!hostMetal || !hasSource || !commandQueue || !sourceBitDepth || !sourceComponents ||
      !outputBitDepth || !outputComponents) {
    return false;
  }

  if (params.processingBackend == kBackendCpu) {
    return false;
  }

  if (params.debugView != static_cast<int>(DebugView::Off) || params.mix <= 0.0001f) {
    return false;
  }

  if (source.storage != ImageStorage::Metal || output.storage != ImageStorage::Metal) {
    return false;
  }

  if (!stringsMatch(sourceBitDepth, kOfxBitDepthFloat) || !stringsMatch(outputBitDepth, kOfxBitDepthFloat) ||
      !stringsMatch(sourceComponents, kOfxImageComponentRGBA) ||
      !stringsMatch(outputComponents, kOfxImageComponentRGBA)) {
    return false;
  }

  return validateMetalImageLayout(source) && validateMetalImageLayout(output);
}

bool canUseMetalCopyForThisRender(const RenderParams &params, bool hostMetal, bool hasSource,
                                  const Image &source, const Image &output, const char *sourceBitDepth,
                                  const char *sourceComponents, const char *outputBitDepth,
                                  const char *outputComponents, void *commandQueue) {
  if (!canUseMetalImagesForThisRender(params,
                                      hostMetal,
                                      hasSource,
                                      source,
                                      output,
                                      sourceBitDepth,
                                      sourceComponents,
                                      outputBitDepth,
                                      outputComponents,
                                      commandQueue)) {
    return false;
  }

  return params.debugView == static_cast<int>(DebugView::Source) ||
         (params.debugView == static_cast<int>(DebugView::Off) && params.mix <= 0.0001f);
}

bool canUseMetalForThisRender(const RenderParams &params, bool hostMetal, bool hasSource,
                              const Image &source, const Image &output, const char *sourceBitDepth,
                              const char *sourceComponents, const char *outputBitDepth,
                              const char *outputComponents, void *commandQueue) {
  if (!canUseMetalImagesForThisRender(params,
                                      hostMetal,
                                      hasSource,
                                      source,
                                      output,
                                      sourceBitDepth,
                                      sourceComponents,
                                      outputBitDepth,
                                      outputComponents,
                                      commandQueue)) {
    return false;
  }

  if (params.debugView != static_cast<int>(DebugView::Off) &&
      !isMetalDiagnosticDebugView(params.debugView)) {
    return false;
  }

  if (params.mix <= 0.0001f && !isMetalDiagnosticDebugView(params.debugView)) {
    return false;
  }

  return true;
}

} // namespace

OfxStatus createInstance(OfxImageEffectHandle instance) {
  if (!instance || !gEffectSuite || !gPropertySuite) {
    return kOfxStatErrBadHandle;
  }

  auto data = std::make_unique<InstanceData>();
  OfxPropertySetHandle props = nullptr;
  if (gEffectSuite->getPropertySet(instance, &props) != kOfxStatOK || !props) {
    return kOfxStatErrBadHandle;
  }

  gPropertySuite->propSetPointer(props, kOfxPropInstanceData, 0, data.get());
  data.release();
  return kOfxStatOK;
}

OfxStatus destroyInstance(OfxImageEffectHandle instance) {
  if (!instance || !gEffectSuite || !gPropertySuite) {
    return kOfxStatErrBadHandle;
  }

  OfxPropertySetHandle props = nullptr;
  if (gEffectSuite->getPropertySet(instance, &props) != kOfxStatOK || !props) {
    return kOfxStatErrBadHandle;
  }

  void *ptr = nullptr;
  gPropertySuite->propGetPointer(props, kOfxPropInstanceData, 0, &ptr);
  delete static_cast<InstanceData *>(ptr);
  gPropertySuite->propSetPointer(props, kOfxPropInstanceData, 0, nullptr);
  return kOfxStatOK;
}

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
  int hostMetalEnabledInt = 0;
  void *hostMetalQueue = nullptr;
#ifdef __APPLE__
  readIntProperty(inArgs, kOfxImageEffectPropMetalEnabled, &hostMetalEnabledInt);
  readPointerProperty(inArgs, kOfxImageEffectPropMetalCommandQueue, &hostMetalQueue);
  if (hostMetalEnabledInt != 0 && !metalRuntimeAvailable()) {
    logMessage(LogLevel::Warn,
               "render",
               "host requested Metal render but packaged Metal runtime is unavailable");
    renderTimer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
    return kOfxStatGPURenderFailed;
  }
#endif

  OfxImageClipHandle sourceClip = nullptr;
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
  logPrintf(LogLevel::Debug,
            "render",
            "clip handles source=%p output=%p",
            sourceClip,
            outputClip);

  OfxPropertySetHandle sourceImageHandle = nullptr;
  OfxPropertySetHandle outputImageHandle = nullptr;
  Image source{};
  Image output{};
  OfxStatus status = kOfxStatOK;
  const bool hostMetal = hostMetalEnabledInt != 0 && hostMetalQueue != nullptr;

  try {
    stage = "fetch_output";
    if (!fetchImage(outputClip, time, hostMetal ? ImageStorage::Metal : ImageStorage::Cpu, &outputImageHandle,
                    &output)) {
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
        const bool hasSource =
            fetchImage(sourceClip, time, hostMetal ? ImageStorage::Metal : ImageStorage::Cpu, &sourceImageHandle,
                       &source);
        char *sourceBitDepth = nullptr;
        char *sourceComponents = nullptr;
        if (hasSource &&
            (!getImageString(sourceImageHandle, kOfxImageEffectPropPixelDepth, &sourceBitDepth) ||
             !getImageString(sourceImageHandle, kOfxImageEffectPropComponents, &sourceComponents) ||
             !stringsMatch(sourceBitDepth, outputBitDepth) ||
             !stringsMatch(sourceComponents, kOfxImageComponentRGBA))) {
          status = kOfxStatErrUnsupported;
          logMessage(LogLevel::Error, "render", "source/output clip format mismatch");
        } else {
          const int sourceBpp = (std::strcmp(outputBitDepth, kOfxBitDepthByte) == 0)
                                    ? static_cast<int>(sizeof(OfxRGBAColourB))
                                    : (std::strcmp(outputBitDepth, kOfxBitDepthShort) == 0)
                                          ? static_cast<int>(sizeof(OfxRGBAColourS))
                                          : static_cast<int>(sizeof(OfxRGBAColourF));
          const bool sourceLayoutValid = hasSource
                                             ? (source.storage == ImageStorage::Metal
                                                    ? validateMetalImageLayout(source)
                                                    : validateCpuImageLayout(source, sourceBpp))
                                             : true;
          const bool outputLayoutValid = output.storage == ImageStorage::Metal
                                             ? validateMetalImageLayout(output)
                                             : validateCpuImageLayout(output, sourceBpp);
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
            RenderParams params = readParams(instance, time);
            logPrintf(LogLevel::Debug,
                      "render",
                      "params mix=%.3f quality=%d lensIdentity=%d",
                      params.mix,
                      params.renderQuality,
                      params.lensIdentity);

            {
              stage = "dispatch_render";
              if (hasSource) {
                params.edgeCropScale = automaticEdgeCropScale(source, source.bounds.x2 - source.bounds.x1,
                                                             source.bounds.y2 - source.bounds.y1, params);
              }
              void *metalCommandQueue = hostMetalQueue;
              const bool useMetalCopy = canUseMetalCopyForThisRender(params,
                                                                     hostMetal,
                                                                     hasSource,
                                                                     source,
                                                                     output,
                                                                     sourceBitDepth,
                                                                     sourceComponents,
                                                                     outputBitDepth,
                                                                     outputComponents,
                                                                     metalCommandQueue);
              const bool useMetal = !useMetalCopy &&
                                    canUseMetalForThisRender(params,
                                                             hostMetal,
                                                             hasSource,
                                                             source,
                                                             output,
                                                             sourceBitDepth,
                                                             sourceComponents,
                                                             outputBitDepth,
                                                             outputComponents,
                                                             metalCommandQueue);
              logPrintf(LogLevel::Info,
                        "render",
                        "render path source=%d sourceStorage=%s outputStorage=%s backend=%s metal=%d metalCopy=%d outputBitDepth=%s",
                        hasSource ? 1 : 0,
                        imageStorageName(source.storage),
                        imageStorageName(output.storage),
                        processingBackendName(params.processingBackend),
                        useMetal ? 1 : 0,
                        useMetalCopy ? 1 : 0,
                        outputBitDepth ? outputBitDepth : "(null)");
              if (useMetalCopy) {
                logPrintf(LogLevel::Warn,
                          "render.gpu",
                          "USING INSTALLED METAL BUILD marker=2026-04-26-copy-test renderQuality=%d debugView=%d mix=%.3f kernel=rimell_copy_float",
                          params.renderQuality,
                          params.debugView,
                          params.mix);
                ScopedLogTimer timer(LogLevel::Info, "render.gpu", "renderMetalCopyFloat");
                timer.setResult("in_progress");
                status = renderMetalCopyFloat(metalCommandQueue, source, output, renderWindow);
                timer.setResult(ofxStatusToString(status));
              } else if (useMetal) {
                ScopedLogTimer timer(LogLevel::Info, "render.gpu", "renderMetalFloat");
                timer.setResult("in_progress");
                status = renderMetalFloat(metalCommandQueue, source, output, renderWindow, params);
                timer.setResult(ofxStatusToString(status));
              } else if (hostMetal) {
                status = kOfxStatGPURenderFailed;
                logMessage(LogLevel::Error,
                           "render",
                           "Metal-enabled images were fetched but the backend gate rejected Metal for this render");
              } else if (std::strcmp(outputBitDepth, kOfxBitDepthByte) == 0) {
                if (hasSource) {
                  logPrintf(LogLevel::Debug,
                            "render",
                            "dispatch cpu pixelType=byte sourceData=%p sourceRowBytes=%d outputData=%p outputRowBytes=%d",
                            source.data,
                            source.rowBytes,
                            output.data,
                            output.rowBytes);
                  status = renderTyped<OfxRGBAColourB>(instance, source, output, renderWindow, params, "byte");
                } else {
                  fillEmptyTyped<OfxRGBAColourB>(output, renderWindow);
                }
              } else if (std::strcmp(outputBitDepth, kOfxBitDepthShort) == 0) {
                if (hasSource) {
                  logPrintf(LogLevel::Debug,
                            "render",
                            "dispatch cpu pixelType=short sourceData=%p sourceRowBytes=%d outputData=%p outputRowBytes=%d",
                            source.data,
                            source.rowBytes,
                            output.data,
                            output.rowBytes);
                  status = renderTyped<OfxRGBAColourS>(instance, source, output, renderWindow, params, "short");
                } else {
                  fillEmptyTyped<OfxRGBAColourS>(output, renderWindow);
                }
              } else if (std::strcmp(outputBitDepth, kOfxBitDepthFloat) == 0) {
                if (hasSource) {
                  logPrintf(LogLevel::Debug,
                            "render",
                            "dispatch cpu pixelType=float sourceData=%p sourceRowBytes=%d outputData=%p outputRowBytes=%d",
                            source.data,
                            source.rowBytes,
                            output.data,
                            output.rowBytes);
                  status = renderTyped<OfxRGBAColourF>(instance, source, output, renderWindow, params, "float");
                } else {
                  fillEmptyTyped<OfxRGBAColourF>(output, renderWindow);
                }
              } else {
                status = kOfxStatErrUnsupported;
                logMessage(LogLevel::Error, "render", "unsupported output bit depth");
              }
              if (status == kOfxStatGPURenderFailed) {
                logMessage(LogLevel::Warn,
                           "render",
                           "Metal render failed; this render will not fall back to CPU because the fetched images may be Metal buffers");
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

  const OfxTime renderTime = readTimeProperty(inArgs);
  const RenderParams params = readParams(instance, renderTime);
  if (params.mix > 0.0001f) {
    return kOfxStatReplyDefault;
  }

  gPropertySuite->propSetString(outArgs, kOfxPropName, 0, kOfxImageEffectSimpleSourceClipName);
  gPropertySuite->propSetDouble(outArgs, kOfxPropTime, 0, renderTime);
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

  const OfxTime time = readTimeProperty(inArgs);
  const RenderParams params = readParams(instance, time);
  const double referenceExtent = std::max(std::abs(roi[2] - roi[0]), std::abs(roi[3] - roi[1]));
  const double padding = static_cast<double>(roiPaddingPixels(params, static_cast<float>(referenceExtent)));
  roi[0] -= padding;
  roi[1] -= padding;
  roi[2] += padding;
  roi[3] += padding;

  const std::string sourceRoi =
      clipPropertyName("OfxImageClipPropRoI_", kOfxImageEffectSimpleSourceClipName);
  gPropertySuite->propSetDoubleN(outArgs, sourceRoi.c_str(), 4, roi);
  return kOfxStatOK;
}

OfxStatus getClipPreferences(OfxImageEffectHandle instance, OfxPropertySetHandle outArgs) {
  if (!outArgs) {
    return kOfxStatReplyDefault;
  }

  const RenderParams params = readParams(instance);
  const bool preferFloat = params.processingBackend != kBackendCpu;

  // OFX clip preference properties are clip-name suffixed by design.
  const std::string outputComponents =
      clipPropertyName("OfxImageClipPropComponents_", kOfxImageEffectOutputClipName);
  const std::string sourceComponents =
      clipPropertyName("OfxImageClipPropComponents_", kOfxImageEffectSimpleSourceClipName);
  gPropertySuite->propSetString(outArgs, outputComponents.c_str(), 0, kOfxImageComponentRGBA);
  gPropertySuite->propSetString(outArgs, sourceComponents.c_str(), 0, kOfxImageComponentRGBA);
  if (preferFloat) {
    const std::string outputDepth =
        clipPropertyName("OfxImageClipPropDepth_", kOfxImageEffectOutputClipName);
    const std::string sourceDepth =
        clipPropertyName("OfxImageClipPropDepth_", kOfxImageEffectSimpleSourceClipName);
    gPropertySuite->propSetString(outArgs, outputDepth.c_str(), 0, kOfxBitDepthFloat);
    gPropertySuite->propSetString(outArgs, sourceDepth.c_str(), 0, kOfxBitDepthFloat);
  }
  gPropertySuite->propSetString(outArgs, kOfxImageEffectPropPreMultiplication, 0,
                                kOfxImageUnPreMultiplied);
  gPropertySuite->propSetInt(outArgs, kOfxImageClipPropContinuousSamples, 0, 0);
  gPropertySuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 0);
  return kOfxStatOK;
}

} // namespace rimell
