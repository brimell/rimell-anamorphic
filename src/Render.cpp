#include "Render.h"

#include "Constants.h"
#include "HostSuites.h"
#include "LensMap.h"
#include "MetalRender.h"
#include "Parameters.h"
#include "PixelAccess.h"
#include "RenderCore.h"

#include "ofxGPURender.h"
#include "ofxPixels.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace rimell {
namespace {

template <typename T>
Pixel warpedSourceSample(const Image &source, float dstX, float dstY, int width, int height,
                         const RenderParams &params, float caPixels) {
  const LensMap map = buildLensMap(dstX, dstY, source, width, height, params);
  Vec2 sourcePixel = lensMapToSourcePixel(map, source, width, height);
  const float caMask = params.edgeOnlyCA > 0.5f ? smoothstep(0.2f, 1.0f, map.radius) : 1.0f;
  sourcePixel.x += map.caDirection.x * caPixels * caMask;
  sourcePixel.y += map.caDirection.y * caPixels * caMask;
  return sampleBilinear<T>(source, sourcePixel.x, sourcePixel.y);
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
                        const RenderParams &params) {
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
    const float radiusPixels = params.longitudinalCA * focusBias * edge * 3.0f;
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
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
  const LensMap map = buildLensMap(x, y, source, width, height, params);
  const float radius = std::max(map.radius, std::sqrt(nx * nx + ny * ny));
  const float edge = std::max(map.edgeMask, smoothstep(std::max(0.0f, 1.0f - params.radialFalloff), 1.15f, radius));

  Pixel result = base;

  const float blurRadius =
      params.edgeBlur * edge * params.edgeBlurPixels + params.fieldCurvature * edge * params.fieldCurvaturePixels;
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
      const float w = std::exp(-std::abs(t) * params.flareFalloff) * h * params.flareIntensity;
      add.r += params.flareColour.r * w;
      add.g += params.flareColour.g * w;
      add.b += params.flareColour.b * w;
    }
  }

  const float bloomPixels = params.bloomRadius * params.bloomPixelScale;
  if ((params.veil > 0.001f || params.highlightCream > 0.001f) && bloomPixels > 0.5f) {
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
      const float w = h * (params.ghostIntensity / static_cast<float>(i)) * tintShift;
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
                      const OfxRectI &renderWindow, const RenderParams &params) {
  const int width = source.bounds.x2 - source.bounds.x1;
  const int height = source.bounds.y2 - source.bounds.y1;
  const bool bypass = params.mix <= 0.0001f;

  for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
    if (gEffectSuite->abort(instance)) {
      return kOfxStatOK;
    }

    for (int x = renderWindow.x1; x < renderWindow.x2; ++x) {
      T *dst = pixelAddress<T>(output, x, y);
      if (!dst) {
        continue;
      }

      const Pixel original = sampleNearest<T>(source, static_cast<float>(x), static_cast<float>(y));
      if (bypass) {
        writePixelTyped(dst, original);
        continue;
      }

      Pixel color = opticalBaseSample<T>(source, static_cast<float>(x), static_cast<float>(y), width, height,
                                         params);
      color = edgeCharacter<T>(source, static_cast<float>(x), static_cast<float>(y), width, height, color,
                               params);
      const Pixel add = lensAdditives<T>(source, static_cast<float>(x), static_cast<float>(y), width, height,
                                         params, color);
      color.r += add.r;
      color.g += add.g;
      color.b += add.b;
      color = applyVignetteAndGuides(color, static_cast<float>(x - source.bounds.x1),
                                     static_cast<float>(y - source.bounds.y1), width, height, params);
      color = composeFinalPixel(original, color, params);
      writePixelTyped(dst, color);
    }
  }

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

bool fetchImage(OfxImageClipHandle clip, OfxTime time, OfxPropertySetHandle *imageHandle, Image *image) {
  if (!clip || !imageHandle || !image) {
    return false;
  }

  if (gEffectSuite->clipGetImage(clip, time, nullptr, imageHandle) != kOfxStatOK || !*imageHandle) {
    return false;
  }

  if (gPropertySuite->propGetPointer(*imageHandle, kOfxImagePropData, 0, &image->data) != kOfxStatOK ||
      gPropertySuite->propGetInt(*imageHandle, kOfxImagePropRowBytes, 0, &image->rowBytes) != kOfxStatOK ||
      gPropertySuite->propGetIntN(*imageHandle, kOfxImagePropBounds, 4, &image->bounds.x1) != kOfxStatOK) {
    return false;
  }
  return image->data != nullptr;
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

bool metalEnabled(OfxPropertySetHandle inArgs, void **commandQueue) {
#ifdef __APPLE__
  int enabled = 0;
  if (!inArgs || !commandQueue ||
      gPropertySuite->propGetInt(inArgs, kOfxImageEffectPropMetalEnabled, 0, &enabled) != kOfxStatOK ||
      enabled == 0) {
    return false;
  }
  return gPropertySuite->propGetPointer(inArgs, kOfxImageEffectPropMetalCommandQueue, 0, commandQueue) ==
             kOfxStatOK &&
         *commandQueue != nullptr;
#else
  (void)inArgs;
  (void)commandQueue;
  return false;
#endif
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
  return std::max({flarePadding, bloomPadding, edgePadding, chromaticPadding, 2.0f});
}

} // namespace

OfxStatus render(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs) {
  OfxTime time = 0.0;
  OfxRectI renderWindow{};
  if (gPropertySuite->propGetDouble(inArgs, kOfxPropTime, 0, &time) != kOfxStatOK ||
      gPropertySuite->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1) != kOfxStatOK) {
    return kOfxStatFailed;
  }

  OfxImageClipHandle sourceClip = nullptr;
  OfxImageClipHandle outputClip = nullptr;
  if (gEffectSuite->clipGetHandle(instance, kOfxImageEffectSimpleSourceClipName, &sourceClip, nullptr) !=
          kOfxStatOK ||
      gEffectSuite->clipGetHandle(instance, kOfxImageEffectOutputClipName, &outputClip, nullptr) != kOfxStatOK ||
      !sourceClip || !outputClip) {
    return kOfxStatErrBadHandle;
  }

  OfxPropertySetHandle sourceImageHandle = nullptr;
  OfxPropertySetHandle outputImageHandle = nullptr;
  Image source;
  Image output;
  OfxStatus status = kOfxStatOK;

  try {
    if (!fetchImage(outputClip, time, &outputImageHandle, &output)) {
      status = gEffectSuite->abort(instance) ? kOfxStatOK : kOfxStatFailed;
    } else {
      char *outputBitDepth = nullptr;
      char *outputComponents = nullptr;
      if (!getImageString(outputImageHandle, kOfxImageEffectPropPixelDepth, &outputBitDepth) ||
          !getImageString(outputImageHandle, kOfxImageEffectPropComponents, &outputComponents) ||
          !stringsMatch(outputComponents, kOfxImageComponentRGBA)) {
        status = kOfxStatErrUnsupported;
      } else {
        const bool hasSource = fetchImage(sourceClip, time, &sourceImageHandle, &source);
        char *sourceBitDepth = nullptr;
        char *sourceComponents = nullptr;
        if (hasSource &&
            (!getImageString(sourceImageHandle, kOfxImageEffectPropPixelDepth, &sourceBitDepth) ||
             !getImageString(sourceImageHandle, kOfxImageEffectPropComponents, &sourceComponents) ||
             !stringsMatch(sourceBitDepth, outputBitDepth) ||
             !stringsMatch(sourceComponents, kOfxImageComponentRGBA))) {
          status = kOfxStatErrUnsupported;
        } else {
          const RenderParams params = readParams(instance);
          void *metalCommandQueue = nullptr;
          const bool useMetal = hasSource && metalEnabled(inArgs, &metalCommandQueue);
          if (useMetal && std::strcmp(outputBitDepth, kOfxBitDepthFloat) == 0) {
            status = renderMetalFloat(metalCommandQueue, source, output, renderWindow, params);
          } else if (useMetal) {
            status = kOfxStatGPURenderFailed;
          } else if (std::strcmp(outputBitDepth, kOfxBitDepthByte) == 0) {
            if (hasSource) {
              status = renderTyped<OfxRGBAColourB>(instance, source, output, renderWindow, params);
            } else {
              fillEmptyTyped<OfxRGBAColourB>(output, renderWindow);
            }
          } else if (std::strcmp(outputBitDepth, kOfxBitDepthShort) == 0) {
            if (hasSource) {
              status = renderTyped<OfxRGBAColourS>(instance, source, output, renderWindow, params);
            } else {
              fillEmptyTyped<OfxRGBAColourS>(output, renderWindow);
            }
          } else if (std::strcmp(outputBitDepth, kOfxBitDepthFloat) == 0) {
            if (hasSource) {
              status = renderTyped<OfxRGBAColourF>(instance, source, output, renderWindow, params);
            } else {
              fillEmptyTyped<OfxRGBAColourF>(output, renderWindow);
            }
          } else {
            status = kOfxStatErrUnsupported;
          }
        }
      }
    }
  } catch (...) {
    status = kOfxStatErrUnknown;
  }

  if (sourceImageHandle) {
    gEffectSuite->clipReleaseImage(sourceImageHandle);
  }
  if (outputImageHandle) {
    gEffectSuite->clipReleaseImage(outputImageHandle);
  }
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
  gPropertySuite->propSetDoubleN(outArgs, sourceRoi.c_str(), 4, roi);
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
  gPropertySuite->propSetString(outArgs, outputComponents.c_str(), 0, kOfxImageComponentRGBA);
  gPropertySuite->propSetString(outArgs, sourceComponents.c_str(), 0, kOfxImageComponentRGBA);
  gPropertySuite->propSetString(outArgs, kOfxImageEffectPropPreMultiplication, 0,
                                kOfxImageUnPreMultiplied);
  gPropertySuite->propSetInt(outArgs, kOfxImageClipPropContinuousSamples, 0, 0);
  gPropertySuite->propSetInt(outArgs, kOfxImageEffectFrameVarying, 0, 0);
  return kOfxStatOK;
}

} // namespace rimell
