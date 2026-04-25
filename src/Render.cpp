#include "Render.h"

#include "Constants.h"
#include "HostSuites.h"
#include "Parameters.h"
#include "PixelAccess.h"

#include "ofxPixels.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace rimell {
namespace {

template <typename T>
Pixel warpedSourceSample(const Image &source, float dstX, float dstY, int width, int height,
                         const RenderParams &params, float caOffset) {
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float halfW = std::max(1.0f, static_cast<float>(width) * 0.5f);
  const float halfH = std::max(1.0f, static_cast<float>(height) * 0.5f);

  float nx = (dstX - cx) / halfW;
  float ny = (dstY - cy) / halfH;
  const float radius2 = nx * nx + ny * ny;
  const float radius = std::sqrt(radius2);

  const float breathe = 1.0f + params.breathingAmount * (0.5f - params.focusDistance) * params.breathingScale;
  nx /= std::max(0.01f, breathe);
  ny /= std::max(0.01f, breathe);

  const float distortion = 1.0f + params.barrel * radius2 + params.mustache * radius2 * radius2;
  nx *= distortion;
  ny *= distortion * (1.0f - params.verticalCompensation * radius2 * params.verticalCompensationScale);

  const float ratio = std::max(0.1f, params.squeezeRatio);
  if (params.squeezeMode == 1) {
    nx *= ratio;
  } else if (params.squeezeMode == 2) {
    nx /= ratio;
  }

  const float fovScale = 1.0f + params.horizontalFovBoost * (70.0f / std::max(10.0f, params.virtualFocalLength));
  nx /= std::max(0.05f, fovScale);

  const float centerWeight = smoothstep(0.9f, 0.0f, radius);
  const float mumps = (params.closeFocusMumps - params.faceWidthCompensation) * centerWeight *
                      smoothstep(1.0f, 0.0f, params.focusDistance);
  nx /= std::max(0.1f, 1.0f + mumps * params.mumpsScale);

  nx *= 1.0f + params.edgeCompression * radius2 * params.edgeCompressionScale;

  const float ca = caOffset * radius * (params.edgeOnlyCA > 0.5f ? smoothstep(0.2f, 1.0f, radius) : 1.0f);
  const float sx = cx + (nx + ca) * halfW;
  const float sy = cy + ny * halfH;

  return sampleBilinear<T>(source, sx, sy);
}

template <typename T>
Pixel opticalBaseSample(const Image &source, float x, float y, int width, int height,
                        const RenderParams &params) {
  const float halfW = std::max(1.0f, static_cast<float>(width) * 0.5f);
  const float caPixels = params.lateralCA * params.lateralCAPixelScale;
  const float caNormalised = caPixels / halfW;
  Pixel base = warpedSourceSample<T>(source, x, y, width, height, params, 0.0f);
  Pixel red = warpedSourceSample<T>(source, x, y, width, height, params, caNormalised);
  Pixel blue = warpedSourceSample<T>(source, x, y, width, height, params, -caNormalised);
  base.r = red.r;
  base.b = blue.b;

  return base;
}

template <typename T>
Pixel edgeCharacter(const Image &source, float x, float y, int width, int height, const Pixel &base,
                    const RenderParams &params) {
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
  const float radius = std::sqrt(nx * nx + ny * ny);
  const float edge = smoothstep(std::max(0.0f, 1.0f - params.radialFalloff), 1.15f, radius);

  Pixel result = base;

  const float blurRadius =
      params.edgeBlur * edge * params.edgeBlurPixels + params.fieldCurvature * edge * params.fieldCurvaturePixels;
  if (blurRadius > 0.05f) {
    Pixel blur{};
    float weight = 0.0f;
    for (int i = -3; i <= 3; ++i) {
      const float t = static_cast<float>(i) / 3.0f;
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
    for (int i = -4; i <= 4; ++i) {
      const float t = static_cast<float>(i) / 4.0f;
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
float highlightAt(const Image &source, float x, float y, float threshold) {
  const Pixel p = sampleBilinear<T>(source, x, y);
  return smoothstep(threshold, 1.0f, luminance(p));
}

template <typename T>
Pixel lensAdditives(const Image &source, float x, float y, int width, int height, const RenderParams &params,
                    const Pixel &base) {
  Pixel add{};

  const float flareAngle = params.flareAngle * kPi / 180.0f;
  const float dirX = std::cos(flareAngle);
  const float dirY = std::sin(flareAngle);
  const int flareSteps = std::max(2, static_cast<int>(2.0f + params.flareLength * params.flareStepDensity));
  const float flareSpan = params.flareLength * static_cast<float>(width) * params.flareSpanScale;
  if (params.flareIntensity > 0.001f && flareSpan > 1.0f) {
    for (int i = -flareSteps; i <= flareSteps; ++i) {
      if (i == 0) {
        continue;
      }
      const float t = static_cast<float>(i) / static_cast<float>(flareSteps);
      const float sx = x + dirX * t * flareSpan;
      const float sy = y + dirY * t * flareSpan;
      const float h = highlightAt<T>(source, sx, sy, params.flareThreshold);
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
    const float stretch = 1.0f + params.bokehStretch * params.bokehStretchScale;
    const int rings = std::max(1, params.bloomRings);
    const int samplesPerRing = std::max(3, params.bloomSamplesPerRing);
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
        const Pixel sample = sampleBilinear<T>(source, x + rx, y + ry);
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
      const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
      const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
      const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
      const float ny = (y - cy) / std::max(1.0f, height * 0.5f);
      const float edge = smoothstep(0.35f, 1.1f, std::sqrt(nx * nx + ny * ny));
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
    for (int i = 1; i <= params.ghostCount; ++i) {
      const float scale = 1.0f + params.ghostSpread * static_cast<float>(i);
      const float sx = cx - (x - cx) * scale;
      const float sy = cy - (y - cy) * scale;
      const Pixel ghost = sampleBilinear<T>(source, sx, sy);
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

Pixel applyVignetteAndGuides(Pixel color, float x, float y, int width, int height, const RenderParams &params) {
  const float cx = (static_cast<float>(width) - 1.0f) * 0.5f;
  const float cy = (static_cast<float>(height) - 1.0f) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);

  const float ovalY = ny * (1.0f + params.ovalVignette * params.ovalVignetteScale);
  const float asym = nx * params.vignetteAsymmetry * params.vignetteAsymmetryScale +
                     ny * params.cornerBias * params.vignetteAsymmetryScale;
  const float vignetteRadius = std::sqrt(nx * nx + ovalY * ovalY) + asym;
  const float vignette = 1.0f - params.ovalVignette * smoothstep(0.35f, 1.2f, vignetteRadius);
  color.r *= vignette;
  color.g *= vignette;
  color.b *= vignette;

  const float edge = smoothstep(0.55f, 1.08f, std::sqrt(nx * nx + ny * ny));
  const float catEye =
      1.0f - params.catEyeStrength * edge * params.catEyeDimScale -
      params.bokehVignette * edge * params.bokehVignetteDimScale;
  color.r *= catEye;
  color.g *= catEye;
  color.b *= catEye;

  if (params.guidesEnabled || params.letterboxPreview) {
    const float target = aspectValue(params.outputAspect, params.customOutputAspect);
    const float current = static_cast<float>(width) / std::max(1.0f, static_cast<float>(height));
    float contentX1 = 0.0f;
    float contentY1 = 0.0f;
    float contentX2 = static_cast<float>(width);
    float contentY2 = static_cast<float>(height);

    if (target > current) {
      const float contentHeight = static_cast<float>(width) / target;
      contentY1 = (static_cast<float>(height) - contentHeight) * 0.5f;
      contentY2 = contentY1 + contentHeight;
    } else {
      const float contentWidth = static_cast<float>(height) * target;
      contentX1 = (static_cast<float>(width) - contentWidth) * 0.5f;
      contentX2 = contentX1 + contentWidth;
    }

    const bool outside = x < contentX1 || x >= contentX2 || y < contentY1 || y >= contentY2;
    if (params.letterboxPreview && outside) {
      const float keep = 1.0f - clamp01(params.letterboxOpacity);
      color.r *= keep;
      color.g *= keep;
      color.b *= keep;
    }

    if (params.guidesEnabled) {
      const float safeInsetX = (contentX2 - contentX1) * (1.0f - params.safeArea) * 0.5f;
      const float safeInsetY = (contentY2 - contentY1) * (1.0f - params.safeArea) * 0.5f;
      const float sx1 = contentX1 + safeInsetX;
      const float sx2 = contentX2 - safeInsetX;
      const float sy1 = contentY1 + safeInsetY;
      const float sy2 = contentY2 - safeInsetY;
      const bool aspectLine = std::abs(x - contentX1) < 1.0f || std::abs(x - contentX2) < 1.0f ||
                              std::abs(y - contentY1) < 1.0f || std::abs(y - contentY2) < 1.0f;
      const bool safeLine = std::abs(x - sx1) < 1.0f || std::abs(x - sx2) < 1.0f ||
                            std::abs(y - sy1) < 1.0f || std::abs(y - sy2) < 1.0f;
      if (aspectLine || safeLine) {
        const float strength = aspectLine ? params.guideAspectStrength : params.guideSafeStrength;
        color.r = lerp(color.r, 1.0f, strength);
        color.g = lerp(color.g, safeLine ? 0.85f : 0.65f, strength);
        color.b = lerp(color.b, 0.25f, strength);
      }
    }
  }

  return color;
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
      color.a = original.a;
      color = lerpPixel(original, color, clamp01(params.mix));
      writePixelTyped(dst, color);
    }
  }

  return kOfxStatOK;
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
    if (!fetchImage(sourceClip, time, &sourceImageHandle, &source) ||
        !fetchImage(outputClip, time, &outputImageHandle, &output)) {
      status = gEffectSuite->abort(instance) ? kOfxStatOK : kOfxStatFailed;
    } else {
      char *sourceBitDepth = nullptr;
      char *outputBitDepth = nullptr;
      char *sourceComponents = nullptr;
      char *outputComponents = nullptr;
      if (!getImageString(sourceImageHandle, kOfxImageEffectPropPixelDepth, &sourceBitDepth) ||
          !getImageString(outputImageHandle, kOfxImageEffectPropPixelDepth, &outputBitDepth) ||
          !getImageString(sourceImageHandle, kOfxImageEffectPropComponents, &sourceComponents) ||
          !getImageString(outputImageHandle, kOfxImageEffectPropComponents, &outputComponents) ||
          !stringsMatch(sourceBitDepth, outputBitDepth) ||
          !stringsMatch(sourceComponents, kOfxImageComponentRGBA) ||
          !stringsMatch(outputComponents, kOfxImageComponentRGBA)) {
        status = kOfxStatErrUnsupported;
      } else {
        const RenderParams params = readParams(instance);

        if (std::strcmp(outputBitDepth, kOfxBitDepthByte) == 0) {
          status = renderTyped<OfxRGBAColourB>(instance, source, output, renderWindow, params);
        } else if (std::strcmp(outputBitDepth, kOfxBitDepthShort) == 0) {
          status = renderTyped<OfxRGBAColourS>(instance, source, output, renderWindow, params);
        } else if (std::strcmp(outputBitDepth, kOfxBitDepthFloat) == 0) {
          status = renderTyped<OfxRGBAColourF>(instance, source, output, renderWindow, params);
        } else {
          status = kOfxStatErrUnsupported;
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

} // namespace rimell
