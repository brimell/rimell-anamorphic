#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxPixels.h"
#include "ofxProperty.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <string>

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__)
#define RIMELL_EXPORT __attribute__((visibility("default")))
#elif defined(_WIN32)
#define RIMELL_EXPORT OfxExport
#else
#error Unsupported platform
#endif

namespace {

constexpr const char *kPluginIdentifier = "com.rimell.ofx.anamorphic";
constexpr int kPluginMajorVersion = 0;
constexpr int kPluginMinorVersion = 1;
constexpr float kPi = 3.14159265358979323846f;

OfxHost *gHost = nullptr;
OfxImageEffectSuiteV1 *gEffectSuite = nullptr;
OfxPropertySuiteV1 *gPropertySuite = nullptr;
OfxParameterSuiteV1 *gParameterSuite = nullptr;

struct Vec3 {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
};

struct Pixel {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 1.0f;
};

struct Image {
  void *data = nullptr;
  OfxRectI bounds{};
  int rowBytes = 0;
};

struct RenderParams {
  float mix = 1.0f;
  int squeezeMode = 2;
  float squeezeRatio = 1.33f;
  float horizontalFovBoost = 0.0f;
  float virtualFocalLength = 50.0f;

  float bokehStretch = 0.15f;
  float bokehRotation = 0.0f;
  float bokehEdgeFalloff = 0.15f;

  float flareIntensity = 0.08f;
  float flareLength = 0.45f;
  Vec3 flareColour{0.35f, 0.75f, 1.0f};
  float flareThreshold = 0.82f;
  float flareAngle = 0.0f;

  float veil = 0.03f;
  float bloomRadius = 0.12f;
  float highlightCream = 0.0f;
  float blackLiftProtection = 0.65f;

  int ghostCount = 0;
  float ghostSpread = 0.35f;
  Vec3 ghostTint{0.55f, 0.8f, 1.0f};
  int coatingStyle = 1;

  float edgeBlur = 0.05f;
  float tangentialSmear = 0.03f;
  float radialFalloff = 0.65f;

  float barrel = 0.0f;
  float mustache = 0.0f;
  float verticalCompensation = 0.0f;

  float closeFocusMumps = 0.0f;
  float faceWidthCompensation = 0.0f;
  float focusDistance = 0.5f;
  float breathingAmount = 0.0f;

  float lateralCA = 0.03f;
  float longitudinalCA = 0.0f;
  float edgeOnlyCA = 1.0f;

  float ovalVignette = 0.05f;
  float vignetteAsymmetry = 0.0f;
  float cornerBias = 0.0f;

  float horizontalSmear = 0.1f;
  float verticalSharpness = 0.0f;
  float fieldCurvature = 0.03f;

  float catEyeStrength = 0.0f;
  float bokehVignette = 0.0f;
  float edgeCompression = 0.0f;

  int guidesEnabled = 0;
  int outputAspect = 0;
  float safeArea = 0.9f;
  int letterboxPreview = 0;
  float letterboxOpacity = 0.55f;
};

float clamp01(float value) {
  return std::max(0.0f, std::min(1.0f, value));
}

float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

Pixel lerpPixel(const Pixel &a, const Pixel &b, float t) {
  return {
      lerp(a.r, b.r, t),
      lerp(a.g, b.g, t),
      lerp(a.b, b.b, t),
      lerp(a.a, b.a, t),
  };
}

float luminance(const Pixel &p) {
  return 0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b;
}

float smoothstep(float edge0, float edge1, float x) {
  if (edge0 == edge1) {
    return x < edge0 ? 0.0f : 1.0f;
  }
  const float t = clamp01((x - edge0) / (edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

int clampCoord(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

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
Pixel readPixelTyped<OfxRGBAColourB>(const OfxRGBAColourB *pixel) {
  if (!pixel) {
    return {};
  }
  constexpr float scale = 1.0f / 255.0f;
  return {pixel->r * scale, pixel->g * scale, pixel->b * scale, pixel->a * scale};
}

template <>
Pixel readPixelTyped<OfxRGBAColourS>(const OfxRGBAColourS *pixel) {
  if (!pixel) {
    return {};
  }
  constexpr float scale = 1.0f / 65535.0f;
  return {pixel->r * scale, pixel->g * scale, pixel->b * scale, pixel->a * scale};
}

template <>
Pixel readPixelTyped<OfxRGBAColourF>(const OfxRGBAColourF *pixel) {
  if (!pixel) {
    return {};
  }
  return {pixel->r, pixel->g, pixel->b, pixel->a};
}

template <typename T>
void writePixelTyped(T *pixel, const Pixel &value);

template <>
void writePixelTyped<OfxRGBAColourB>(OfxRGBAColourB *pixel, const Pixel &value) {
  pixel->r = static_cast<unsigned char>(std::lround(clamp01(value.r) * 255.0f));
  pixel->g = static_cast<unsigned char>(std::lround(clamp01(value.g) * 255.0f));
  pixel->b = static_cast<unsigned char>(std::lround(clamp01(value.b) * 255.0f));
  pixel->a = static_cast<unsigned char>(std::lround(clamp01(value.a) * 255.0f));
}

template <>
void writePixelTyped<OfxRGBAColourS>(OfxRGBAColourS *pixel, const Pixel &value) {
  pixel->r = static_cast<unsigned short>(std::lround(clamp01(value.r) * 65535.0f));
  pixel->g = static_cast<unsigned short>(std::lround(clamp01(value.g) * 65535.0f));
  pixel->b = static_cast<unsigned short>(std::lround(clamp01(value.b) * 65535.0f));
  pixel->a = static_cast<unsigned short>(std::lround(clamp01(value.a) * 65535.0f));
}

template <>
void writePixelTyped<OfxRGBAColourF>(OfxRGBAColourF *pixel, const Pixel &value) {
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

double getDoubleParam(OfxParamSetHandle paramSet, const char *name, double fallback) {
  OfxParamHandle handle = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr) != kOfxStatOK || !handle) {
    return fallback;
  }

  double value = fallback;
  gParameterSuite->paramGetValue(handle, &value);
  return value;
}

int getIntParam(OfxParamSetHandle paramSet, const char *name, int fallback) {
  OfxParamHandle handle = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr) != kOfxStatOK || !handle) {
    return fallback;
  }

  int value = fallback;
  gParameterSuite->paramGetValue(handle, &value);
  return value;
}

Vec3 getRGBParam(OfxParamSetHandle paramSet, const char *name, Vec3 fallback) {
  OfxParamHandle handle = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr) != kOfxStatOK || !handle) {
    return fallback;
  }

  double r = fallback.r;
  double g = fallback.g;
  double b = fallback.b;
  gParameterSuite->paramGetValue(handle, &r, &g, &b);
  return {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};
}

RenderParams readParams(OfxImageEffectHandle effect) {
  OfxParamSetHandle paramSet = nullptr;
  gEffectSuite->getParamSet(effect, &paramSet);

  RenderParams params;
  params.mix = static_cast<float>(getDoubleParam(paramSet, "mix", params.mix));
  params.squeezeMode = getIntParam(paramSet, "squeezeMode", params.squeezeMode);
  params.squeezeRatio = static_cast<float>(getDoubleParam(paramSet, "squeezeRatio", params.squeezeRatio));
  params.horizontalFovBoost =
      static_cast<float>(getDoubleParam(paramSet, "horizontalFovBoost", params.horizontalFovBoost));
  params.virtualFocalLength =
      static_cast<float>(getDoubleParam(paramSet, "virtualFocalLength", params.virtualFocalLength));

  params.bokehStretch = static_cast<float>(getDoubleParam(paramSet, "bokehStretch", params.bokehStretch));
  params.bokehRotation = static_cast<float>(getDoubleParam(paramSet, "bokehRotation", params.bokehRotation));
  params.bokehEdgeFalloff =
      static_cast<float>(getDoubleParam(paramSet, "bokehEdgeFalloff", params.bokehEdgeFalloff));

  params.flareIntensity =
      static_cast<float>(getDoubleParam(paramSet, "flareIntensity", params.flareIntensity));
  params.flareLength = static_cast<float>(getDoubleParam(paramSet, "flareLength", params.flareLength));
  params.flareColour = getRGBParam(paramSet, "flareColour", params.flareColour);
  params.flareThreshold = static_cast<float>(getDoubleParam(paramSet, "flareThreshold", params.flareThreshold));
  params.flareAngle = static_cast<float>(getDoubleParam(paramSet, "flareAngle", params.flareAngle));

  params.veil = static_cast<float>(getDoubleParam(paramSet, "veil", params.veil));
  params.bloomRadius = static_cast<float>(getDoubleParam(paramSet, "bloomRadius", params.bloomRadius));
  params.highlightCream =
      static_cast<float>(getDoubleParam(paramSet, "highlightCream", params.highlightCream));
  params.blackLiftProtection =
      static_cast<float>(getDoubleParam(paramSet, "blackLiftProtection", params.blackLiftProtection));

  params.ghostCount = getIntParam(paramSet, "ghostCount", params.ghostCount);
  params.ghostSpread = static_cast<float>(getDoubleParam(paramSet, "ghostSpread", params.ghostSpread));
  params.ghostTint = getRGBParam(paramSet, "ghostTint", params.ghostTint);
  params.coatingStyle = getIntParam(paramSet, "coatingStyle", params.coatingStyle);

  params.edgeBlur = static_cast<float>(getDoubleParam(paramSet, "edgeBlur", params.edgeBlur));
  params.tangentialSmear =
      static_cast<float>(getDoubleParam(paramSet, "tangentialSmear", params.tangentialSmear));
  params.radialFalloff = static_cast<float>(getDoubleParam(paramSet, "radialFalloff", params.radialFalloff));

  params.barrel = static_cast<float>(getDoubleParam(paramSet, "barrel", params.barrel));
  params.mustache = static_cast<float>(getDoubleParam(paramSet, "mustache", params.mustache));
  params.verticalCompensation =
      static_cast<float>(getDoubleParam(paramSet, "verticalCompensation", params.verticalCompensation));

  params.closeFocusMumps =
      static_cast<float>(getDoubleParam(paramSet, "closeFocusMumps", params.closeFocusMumps));
  params.faceWidthCompensation = static_cast<float>(
      getDoubleParam(paramSet, "faceWidthCompensation", params.faceWidthCompensation));
  params.focusDistance = static_cast<float>(getDoubleParam(paramSet, "focusDistance", params.focusDistance));
  params.breathingAmount =
      static_cast<float>(getDoubleParam(paramSet, "breathingAmount", params.breathingAmount));

  params.lateralCA = static_cast<float>(getDoubleParam(paramSet, "lateralCA", params.lateralCA));
  params.longitudinalCA =
      static_cast<float>(getDoubleParam(paramSet, "longitudinalCA", params.longitudinalCA));
  params.edgeOnlyCA = static_cast<float>(getIntParam(paramSet, "edgeOnlyCA", static_cast<int>(params.edgeOnlyCA)));

  params.ovalVignette = static_cast<float>(getDoubleParam(paramSet, "ovalVignette", params.ovalVignette));
  params.vignetteAsymmetry =
      static_cast<float>(getDoubleParam(paramSet, "vignetteAsymmetry", params.vignetteAsymmetry));
  params.cornerBias = static_cast<float>(getDoubleParam(paramSet, "cornerBias", params.cornerBias));

  params.horizontalSmear =
      static_cast<float>(getDoubleParam(paramSet, "horizontalSmear", params.horizontalSmear));
  params.verticalSharpness =
      static_cast<float>(getDoubleParam(paramSet, "verticalSharpness", params.verticalSharpness));
  params.fieldCurvature =
      static_cast<float>(getDoubleParam(paramSet, "fieldCurvature", params.fieldCurvature));

  params.catEyeStrength =
      static_cast<float>(getDoubleParam(paramSet, "catEyeStrength", params.catEyeStrength));
  params.bokehVignette =
      static_cast<float>(getDoubleParam(paramSet, "bokehVignette", params.bokehVignette));
  params.edgeCompression =
      static_cast<float>(getDoubleParam(paramSet, "edgeCompression", params.edgeCompression));

  params.guidesEnabled = getIntParam(paramSet, "guidesEnabled", params.guidesEnabled);
  params.outputAspect = getIntParam(paramSet, "outputAspect", params.outputAspect);
  params.safeArea = static_cast<float>(getDoubleParam(paramSet, "safeArea", params.safeArea));
  params.letterboxPreview = getIntParam(paramSet, "letterboxPreview", params.letterboxPreview);
  params.letterboxOpacity =
      static_cast<float>(getDoubleParam(paramSet, "letterboxOpacity", params.letterboxOpacity));

  return params;
}

float aspectValue(int index) {
  switch (index) {
  case 0:
    return 2.0f;
  case 1:
    return 2.39f;
  case 2:
    return 2.66f;
  default:
    return 2.39f;
  }
}

void addDoubleParam(OfxParamSetHandle paramSet, const char *name, const char *label, double defaultValue,
                    double minValue, double maxValue, double displayMin, double displayMax,
                    const char *hint = nullptr) {
  OfxPropertySetHandle props = nullptr;
  gParameterSuite->paramDefine(paramSet, kOfxParamTypeDouble, name, &props);
  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 0, defaultValue);
  gPropertySuite->propSetDouble(props, kOfxParamPropMin, 0, minValue);
  gPropertySuite->propSetDouble(props, kOfxParamPropMax, 0, maxValue);
  gPropertySuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, displayMin);
  gPropertySuite->propSetDouble(props, kOfxParamPropDisplayMax, 0, displayMax);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
}

void addIntParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                 int minValue, int maxValue, const char *hint = nullptr) {
  OfxPropertySetHandle props = nullptr;
  gParameterSuite->paramDefine(paramSet, kOfxParamTypeInteger, name, &props);
  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropDefault, 0, defaultValue);
  gPropertySuite->propSetInt(props, kOfxParamPropMin, 0, minValue);
  gPropertySuite->propSetInt(props, kOfxParamPropMax, 0, maxValue);
  gPropertySuite->propSetInt(props, kOfxParamPropDisplayMin, 0, minValue);
  gPropertySuite->propSetInt(props, kOfxParamPropDisplayMax, 0, maxValue);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
}

void addBooleanParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                     const char *hint = nullptr) {
  OfxPropertySetHandle props = nullptr;
  gParameterSuite->paramDefine(paramSet, kOfxParamTypeBoolean, name, &props);
  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropDefault, 0, defaultValue);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
}

void addChoiceParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                    const char *option0, const char *option1, const char *option2 = nullptr,
                    const char *option3 = nullptr, const char *hint = nullptr) {
  OfxPropertySetHandle props = nullptr;
  gParameterSuite->paramDefine(paramSet, kOfxParamTypeChoice, name, &props);
  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropDefault, 0, defaultValue);
  gPropertySuite->propSetString(props, kOfxParamPropChoiceOption, 0, option0);
  gPropertySuite->propSetString(props, kOfxParamPropChoiceOption, 1, option1);
  if (option2) {
    gPropertySuite->propSetString(props, kOfxParamPropChoiceOption, 2, option2);
  }
  if (option3) {
    gPropertySuite->propSetString(props, kOfxParamPropChoiceOption, 3, option3);
  }
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
}

void addRGBParam(OfxParamSetHandle paramSet, const char *name, const char *label, Vec3 defaultValue,
                 const char *hint = nullptr) {
  OfxPropertySetHandle props = nullptr;
  gParameterSuite->paramDefine(paramSet, kOfxParamTypeRGB, name, &props);
  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 0, defaultValue.r);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 1, defaultValue.g);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 2, defaultValue.b);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
}

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

  const float breathe = 1.0f + params.breathingAmount * (0.5f - params.focusDistance) * 0.12f;
  nx /= std::max(0.01f, breathe);
  ny /= std::max(0.01f, breathe);

  const float distortion = 1.0f + params.barrel * radius2 + params.mustache * radius2 * radius2;
  nx *= distortion;
  ny *= distortion * (1.0f - params.verticalCompensation * radius2 * 0.35f);

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
  nx /= std::max(0.1f, 1.0f + mumps * 0.28f);

  nx *= 1.0f + params.edgeCompression * radius2 * 0.16f;

  const float ca = caOffset * radius * (params.edgeOnlyCA > 0.5f ? smoothstep(0.2f, 1.0f, radius) : 1.0f);
  const float sx = cx + (nx + ca) * halfW;
  const float sy = cy + ny * halfH;

  return sampleBilinear<T>(source, sx, sy);
}

template <typename T>
Pixel opticalBaseSample(const Image &source, float x, float y, int width, int height,
                        const RenderParams &params) {
  const float halfW = std::max(1.0f, static_cast<float>(width) * 0.5f);
  const float caPixels = params.lateralCA * 4.0f;
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

  const float blurRadius = params.edgeBlur * edge * 10.0f + params.fieldCurvature * edge * 4.0f;
  if (blurRadius > 0.05f) {
    Pixel blur{};
    float weight = 0.0f;
    for (int i = -3; i <= 3; ++i) {
      const float t = static_cast<float>(i) / 3.0f;
      const float w = 1.0f - std::abs(t) * 0.55f;
      const Pixel sample = opticalBaseSample<T>(source, x + t * blurRadius, y + t * blurRadius * 0.25f, width,
                                                height, params);
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

  const float smearRadius = edge * (params.tangentialSmear + params.horizontalSmear) * 18.0f;
  if (smearRadius > 0.05f) {
    Pixel smear{};
    float weight = 0.0f;
    for (int i = -4; i <= 4; ++i) {
      const float t = static_cast<float>(i) / 4.0f;
      const float w = 1.0f - std::abs(t) * 0.7f;
      const Pixel sample = opticalBaseSample<T>(source, x + t * smearRadius, y, width, height, params);
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
    const Pixel up = opticalBaseSample<T>(source, x, y - 1.5f, width, height, params);
    const Pixel down = opticalBaseSample<T>(source, x, y + 1.5f, width, height, params);
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
  const int flareSteps = std::max(2, static_cast<int>(2 + params.flareLength * 6.0f));
  const float flareSpan = params.flareLength * static_cast<float>(width) * 0.75f;
  if (params.flareIntensity > 0.001f && flareSpan > 1.0f) {
    for (int i = -flareSteps; i <= flareSteps; ++i) {
      if (i == 0) {
        continue;
      }
      const float t = static_cast<float>(i) / static_cast<float>(flareSteps);
      const float sx = x + dirX * t * flareSpan;
      const float sy = y + dirY * t * flareSpan;
      const float h = highlightAt<T>(source, sx, sy, params.flareThreshold);
      const float w = std::exp(-std::abs(t) * 3.0f) * h * params.flareIntensity;
      add.r += params.flareColour.r * w;
      add.g += params.flareColour.g * w;
      add.b += params.flareColour.b * w;
    }
  }

  const float bloomPixels = params.bloomRadius * 80.0f;
  if ((params.veil > 0.001f || params.highlightCream > 0.001f) && bloomPixels > 0.5f) {
    const float rotation = params.bokehRotation * kPi / 180.0f;
    const float cosR = std::cos(rotation);
    const float sinR = std::sin(rotation);
    const float stretch = 1.0f + params.bokehStretch * 2.2f;
    const int rings = 2;
    const int samplesPerRing = 6;
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
        const float h = smoothstep(params.flareThreshold * 0.75f, 1.0f, luminance(sample));
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
      const float bokehEdgeKeep = 1.0f - edge * params.bokehEdgeFalloff * 0.45f;
      const float protect = lerp(1.0f, clamp01(luminance(base) * 2.0f), params.blackLiftProtection);
      const float amount = (params.veil * 0.4f + params.highlightCream * 0.8f) * protect * bokehEdgeKeep;
      add.r += bloom.r * amount;
      add.g += bloom.g * amount;
      add.b += bloom.b * amount;
    }
  }

  if (params.ghostCount > 0 && params.ghostSpread > 0.001f) {
    const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
    const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
    const float tintShift = params.coatingStyle == 0 ? 0.75f : (params.coatingStyle == 2 ? 1.25f : 1.0f);
    for (int i = 1; i <= params.ghostCount; ++i) {
      const float scale = 1.0f + params.ghostSpread * static_cast<float>(i);
      const float sx = cx - (x - cx) * scale;
      const float sy = cy - (y - cy) * scale;
      const Pixel ghost = sampleBilinear<T>(source, sx, sy);
      const float h = smoothstep(params.flareThreshold, 1.0f, luminance(ghost));
      const float w = h * (0.13f / static_cast<float>(i)) * tintShift;
      add.r += ghost.r * params.ghostTint.r * w;
      add.g += ghost.g * params.ghostTint.g * w;
      add.b += ghost.b * params.ghostTint.b * w;
    }
  }

  const float centerGlow = smoothstep(params.flareThreshold * 0.9f, 1.0f, luminance(base));
  add.r += params.veil * centerGlow * 0.08f;
  add.g += params.veil * centerGlow * 0.08f;
  add.b += params.veil * centerGlow * 0.08f;

  return add;
}

Pixel applyVignetteAndGuides(Pixel color, float x, float y, int width, int height, const RenderParams &params) {
  const float cx = (static_cast<float>(width) - 1.0f) * 0.5f;
  const float cy = (static_cast<float>(height) - 1.0f) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);

  const float ovalY = ny * (1.0f + params.ovalVignette * 1.8f);
  const float asym = nx * params.vignetteAsymmetry * 0.35f + ny * params.cornerBias * 0.35f;
  const float vignetteRadius = std::sqrt(nx * nx + ovalY * ovalY) + asym;
  const float vignette = 1.0f - params.ovalVignette * smoothstep(0.35f, 1.2f, vignetteRadius);
  color.r *= vignette;
  color.g *= vignette;
  color.b *= vignette;

  const float edge = smoothstep(0.55f, 1.08f, std::sqrt(nx * nx + ny * ny));
  const float catEye = 1.0f - params.catEyeStrength * edge * 0.22f - params.bokehVignette * edge * 0.18f;
  color.r *= catEye;
  color.g *= catEye;
  color.b *= catEye;

  if (params.guidesEnabled || params.letterboxPreview) {
    const float target = aspectValue(params.outputAspect);
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
        const float strength = aspectLine ? 0.85f : 0.45f;
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

OfxStatus describe(OfxImageEffectHandle effect) {
  OfxPropertySetHandle props = nullptr;
  gEffectSuite->getPropertySet(effect, &props);

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, "Rimell Anamorphic");
  gPropertySuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Rimell/Lens");
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthFloat);
  gPropertySuite->propSetInt(props, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);
  gPropertySuite->propSetInt(props, kOfxImageEffectPluginPropSingleInstance, 0, 0);
  gPropertySuite->propSetInt(props, kOfxImageEffectPluginPropHostFrameThreading, 0, 1);
  gPropertySuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0,
                                kOfxImageEffectRenderFullySafe);
  gPropertySuite->propSetInt(props, kOfxImageEffectPropSupportsTiles, 0, 0);
  gPropertySuite->propSetInt(props, kOfxImageEffectPropTemporalClipAccess, 0, 0);
  return kOfxStatOK;
}

OfxStatus describeInContext(OfxImageEffectHandle effect) {
  OfxPropertySetHandle props = nullptr;

  gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

  gEffectSuite->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

  OfxParamSetHandle paramSet = nullptr;
  gEffectSuite->getParamSet(effect, &paramSet);

  addDoubleParam(paramSet, "mix", "Mix", 1.0, 0.0, 1.0, 0.0, 1.0);

  addChoiceParam(paramSet, "squeezeMode", "Squeeze Mode", 2, "Off", "Squeeze", "Desqueeze",
                 nullptr, "Squeeze simulates capture compression; desqueeze stretches horizontally.");
  addDoubleParam(paramSet, "squeezeRatio", "Squeeze Ratio", 1.33, 1.0, 2.0, 1.0, 2.0);
  addDoubleParam(paramSet, "horizontalFovBoost", "Horizontal FOV Boost", 0.0, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "virtualFocalLength", "Virtual Focal Length", 50.0, 10.0, 200.0, 18.0, 100.0);

  addDoubleParam(paramSet, "bokehStretch", "Anamorphic Bloom Shape", 0.15, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "bokehRotation", "Bloom Shape Rotation", 0.0, -45.0, 45.0, -15.0, 15.0);
  addDoubleParam(paramSet, "bokehEdgeFalloff", "Bloom Edge Falloff", 0.15, 0.0, 1.0, 0.0, 1.0);

  addDoubleParam(paramSet, "flareIntensity", "Flare Intensity", 0.08, 0.0, 4.0, 0.0, 1.5);
  addDoubleParam(paramSet, "flareLength", "Flare Length", 0.45, 0.0, 1.0, 0.0, 1.0);
  addRGBParam(paramSet, "flareColour", "Flare Colour", {0.35f, 0.75f, 1.0f});
  addDoubleParam(paramSet, "flareThreshold", "Flare Threshold", 0.82, 0.0, 1.0, 0.4, 1.0);
  addDoubleParam(paramSet, "flareAngle", "Flare Angle", 0.0, -45.0, 45.0, -15.0, 15.0);

  addDoubleParam(paramSet, "veil", "Veil", 0.03, 0.0, 1.0, 0.0, 0.5);
  addDoubleParam(paramSet, "bloomRadius", "Bloom Radius", 0.12, 0.0, 1.0, 0.0, 0.7);
  addDoubleParam(paramSet, "highlightCream", "Highlight Cream", 0.0, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "blackLiftProtection", "Black Lift Protection", 0.65, 0.0, 1.0, 0.0, 1.0);

  addIntParam(paramSet, "ghostCount", "Ghost Count", 0, 0, 8);
  addDoubleParam(paramSet, "ghostSpread", "Ghost Spread", 0.35, 0.0, 1.0, 0.0, 1.0);
  addRGBParam(paramSet, "ghostTint", "Ghost Tint", {0.55f, 0.8f, 1.0f});
  addChoiceParam(paramSet, "coatingStyle", "Coating Style", 1, "Warm", "Neutral", "Cool");

  addDoubleParam(paramSet, "edgeBlur", "Edge Blur", 0.05, 0.0, 1.0, 0.0, 0.7);
  addDoubleParam(paramSet, "tangentialSmear", "Tangential Smear", 0.03, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "radialFalloff", "Radial Falloff", 0.65, 0.0, 1.0, 0.0, 1.0);

  addDoubleParam(paramSet, "barrel", "Barrel", 0.0, -0.5, 0.5, -0.2, 0.2);
  addDoubleParam(paramSet, "mustache", "Mustache", 0.0, -0.5, 0.5, -0.2, 0.2);
  addDoubleParam(paramSet, "verticalCompensation", "Vertical Compensation", 0.0, -1.0, 1.0, -0.5, 0.5);

  addDoubleParam(paramSet, "closeFocusMumps", "Close Focus Mumps", 0.0, 0.0, 1.0, 0.0, 0.6);
  addDoubleParam(paramSet, "faceWidthCompensation", "Face Width Compensation", 0.0, 0.0, 1.0, 0.0, 0.6);
  addDoubleParam(paramSet, "focusDistance", "Focus Distance", 0.5, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "breathingAmount", "Breathing Amount", 0.0, -1.0, 1.0, -0.5, 0.5);

  addDoubleParam(paramSet, "lateralCA", "Lateral CA", 0.03, 0.0, 1.0, 0.0, 0.6,
                 "1.0 is approximately four pixels of edge separation.");
  addDoubleParam(paramSet, "longitudinalCA", "Longitudinal CA", 0.0, 0.0, 1.0, 0.0, 0.3,
                 "Reserved for a future warped-coordinate implementation.");
  addBooleanParam(paramSet, "edgeOnlyCA", "Edge Only CA", 1);

  addDoubleParam(paramSet, "ovalVignette", "Oval Vignette", 0.05, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "vignetteAsymmetry", "Asymmetry", 0.0, -1.0, 1.0, -0.5, 0.5);
  addDoubleParam(paramSet, "cornerBias", "Corner Bias", 0.0, -1.0, 1.0, -0.5, 0.5);

  addDoubleParam(paramSet, "horizontalSmear", "Horizontal Smear", 0.1, 0.0, 1.0, 0.0, 0.7);
  addDoubleParam(paramSet, "verticalSharpness", "Vertical Sharpness", 0.0, 0.0, 1.0, 0.0, 0.5);
  addDoubleParam(paramSet, "fieldCurvature", "Field Curvature", 0.03, 0.0, 1.0, 0.0, 0.7);

  addDoubleParam(paramSet, "catEyeStrength", "Edge Highlight Vignette", 0.0, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "bokehVignette", "Bloom Vignette", 0.0, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "edgeCompression", "Edge Compression", 0.0, 0.0, 1.0, 0.0, 0.7);

  addBooleanParam(paramSet, "guidesEnabled", "Aspect Guides", 0);
  addChoiceParam(paramSet, "outputAspect", "Output Aspect", 1, "2.00:1", "2.39:1", "2.66:1");
  addDoubleParam(paramSet, "safeArea", "Safe Area", 0.9, 0.5, 1.0, 0.8, 1.0);
  addBooleanParam(paramSet, "letterboxPreview", "Letterbox Preview", 0);
  addDoubleParam(paramSet, "letterboxOpacity", "Letterbox Opacity", 0.55, 0.0, 1.0, 0.0, 1.0);

  return kOfxStatOK;
}

OfxStatus onLoad() {
  if (!gHost) {
    return kOfxStatErrMissingHostFeature;
  }

  gEffectSuite = const_cast<OfxImageEffectSuiteV1 *>(
      static_cast<const OfxImageEffectSuiteV1 *>(gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1)));
  gPropertySuite = const_cast<OfxPropertySuiteV1 *>(
      static_cast<const OfxPropertySuiteV1 *>(gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1)));
  gParameterSuite = const_cast<OfxParameterSuiteV1 *>(
      static_cast<const OfxParameterSuiteV1 *>(gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1)));

  if (!gEffectSuite || !gPropertySuite || !gParameterSuite) {
    return kOfxStatErrMissingHostFeature;
  }

  return kOfxStatOK;
}

OfxStatus pluginMain(const char *action, const void *handle, OfxPropertySetHandle inArgs,
                     OfxPropertySetHandle /*outArgs*/) {
  try {
    auto effect = static_cast<OfxImageEffectHandle>(const_cast<void *>(handle));

    if (std::strcmp(action, kOfxActionLoad) == 0) {
      return onLoad();
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
      return describe(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
      return describeInContext(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
      return render(effect, inArgs);
    }
  } catch (const std::bad_alloc &) {
    return kOfxStatErrMemory;
  } catch (const std::exception &) {
    return kOfxStatErrUnknown;
  } catch (...) {
    return kOfxStatErrUnknown;
  }

  return kOfxStatReplyDefault;
}

void setHost(OfxHost *host) {
  gHost = host;
}

OfxPlugin plugin = {
    kOfxImageEffectPluginApi,
    1,
    kPluginIdentifier,
    kPluginMajorVersion,
    kPluginMinorVersion,
    setHost,
    pluginMain,
};

} // namespace

extern "C" {

RIMELL_EXPORT OfxPlugin *OfxGetPlugin(int nth) {
  return nth == 0 ? &plugin : nullptr;
}

RIMELL_EXPORT int OfxGetNumberOfPlugins() {
  return 1;
}
}
