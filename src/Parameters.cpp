#include "Parameters.h"

#include "Diagnostics.h"
#include "HostSuites.h"
#include "ParameterLogic.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <unordered_set>

namespace rimell {

namespace {

std::mutex gParamLogMutex;
std::unordered_set<std::string> gLoggedParamIssues;

void logParamIssueOnce(const char *kind, const char *name, const char *detail) {
  if (!kind || !name || !detail) {
    return;
  }

  const std::string key = std::string(kind) + ":" + name + ":" + detail;
  {
    std::lock_guard<std::mutex> lock(gParamLogMutex);
    if (!gLoggedParamIssues.insert(key).second) {
      return;
    }
  }

  logPrintf(LogLevel::Warn, "params", "%s '%s': %s", kind, name, detail);
}

void logMissingParamOnce(const char *name, OfxStatus status) {
  logParamIssueOnce("missing param", name, ofxStatusToString(status));
}

void logParamReadFailureOnce(const char *name, OfxStatus status) {
  logParamIssueOnce("param read failed", name, ofxStatusToString(status));
}

void logSchemaVersionMismatchOnce(int schemaVersion) {
  const std::string detail = "project schema " + std::to_string(schemaVersion) +
                             " older than current schema " + std::to_string(kCurrentParameterSchemaVersion);
  logParamIssueOnce("schema version mismatch", "schemaVersion", detail.c_str());
}

} // namespace

double getDoubleParamAtTime(OfxParamSetHandle paramSet, const char *name, OfxTime time, double fallback);
int getIntParamAtTime(OfxParamSetHandle paramSet, const char *name, OfxTime time, int fallback);
Vec3 getRGBParamAtTime(OfxParamSetHandle paramSet, const char *name, OfxTime time, Vec3 fallback);
RenderParams readParams(OfxImageEffectHandle effect, OfxTime time);

double getDoubleParam(OfxParamSetHandle paramSet, const char *name, double fallback) {
  return getDoubleParamAtTime(paramSet, name, 0.0, fallback);
}

double getDoubleParamAtTime(OfxParamSetHandle paramSet, const char *name, OfxTime time, double fallback) {
  if (!paramSet || !name || !gParameterSuite) {
    return fallback;
  }

  OfxParamHandle handle = nullptr;
  const OfxStatus handleStatus = gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr);
  if (handleStatus != kOfxStatOK || !handle) {
    logMissingParamOnce(name, handleStatus);
    return fallback;
  }

  double value = fallback;
  const OfxStatus valueStatus = gParameterSuite->paramGetValueAtTime
                                    ? gParameterSuite->paramGetValueAtTime(handle, time, &value)
                                    : gParameterSuite->paramGetValue(handle, &value);
  if (valueStatus != kOfxStatOK) {
    logParamReadFailureOnce(name, valueStatus);
    return fallback;
  }
  return value;
}

int getIntParam(OfxParamSetHandle paramSet, const char *name, int fallback) {
  return getIntParamAtTime(paramSet, name, 0.0, fallback);
}

int getIntParamAtTime(OfxParamSetHandle paramSet, const char *name, OfxTime time, int fallback) {
  if (!paramSet || !name || !gParameterSuite) {
    return fallback;
  }

  OfxParamHandle handle = nullptr;
  const OfxStatus handleStatus = gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr);
  if (handleStatus != kOfxStatOK || !handle) {
    logMissingParamOnce(name, handleStatus);
    return fallback;
  }

  int value = fallback;
  const OfxStatus valueStatus = gParameterSuite->paramGetValueAtTime
                                    ? gParameterSuite->paramGetValueAtTime(handle, time, &value)
                                    : gParameterSuite->paramGetValue(handle, &value);
  if (valueStatus != kOfxStatOK) {
    logParamReadFailureOnce(name, valueStatus);
    return fallback;
  }
  return value;
}

Vec3 getRGBParam(OfxParamSetHandle paramSet, const char *name, Vec3 fallback) {
  return getRGBParamAtTime(paramSet, name, 0.0, fallback);
}

Vec3 getRGBParamAtTime(OfxParamSetHandle paramSet, const char *name, OfxTime time, Vec3 fallback) {
  if (!paramSet || !name || !gParameterSuite) {
    return fallback;
  }

  OfxParamHandle handle = nullptr;
  const OfxStatus handleStatus = gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr);
  if (handleStatus != kOfxStatOK || !handle) {
    logMissingParamOnce(name, handleStatus);
    return fallback;
  }

  double r = fallback.r;
  double g = fallback.g;
  double b = fallback.b;
  const OfxStatus valueStatus = gParameterSuite->paramGetValueAtTime
                                    ? gParameterSuite->paramGetValueAtTime(handle, time, &r, &g, &b)
                                    : gParameterSuite->paramGetValue(handle, &r, &g, &b);
  if (valueStatus != kOfxStatOK) {
    logParamReadFailureOnce(name, valueStatus);
    return fallback;
  }
  return {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};
}

RenderParams readParams(OfxImageEffectHandle effect) {
  return readParams(effect, 0.0);
}

RenderParams readParams(OfxImageEffectHandle effect, OfxTime time) {
  OfxParamSetHandle paramSet = nullptr;
  if (!effect || !gEffectSuite || gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK || !paramSet) {
    return normalizeRenderParams(RenderParams{});
  }

  RenderParams params;
  params.schemaVersion = getIntParamAtTime(paramSet, "schemaVersion", time, params.schemaVersion);
  if (params.schemaVersion < kCurrentParameterSchemaVersion) {
    logSchemaVersionMismatchOnce(params.schemaVersion);
  }
  params.mix = static_cast<float>(getDoubleParamAtTime(paramSet, "mix", time, params.mix));
  params.debugView = getIntParamAtTime(paramSet, "debugView", time, params.debugView);
  params.renderQuality = getIntParamAtTime(paramSet, "renderQuality", time, params.renderQuality);
  params.processingBackend = getIntParamAtTime(paramSet, "processingBackend", time, params.processingBackend);
  params.lookPreset = getIntParamAtTime(paramSet, "lookPreset", time, params.lookPreset);
  params.inputMode = getIntParamAtTime(paramSet, "inputMode", time, params.inputMode);
  params.squeezeMode = getIntParamAtTime(paramSet, "squeezeMode", time, params.squeezeMode);
  params.anamorphicTransfer =
      static_cast<float>(getDoubleParamAtTime(paramSet, "anamorphicTransfer", time, params.anamorphicTransfer));
  params.lensIdentity = getIntParamAtTime(paramSet, "lensIdentity", time, params.lensIdentity);
  params.squeezeRatio = static_cast<float>(getDoubleParamAtTime(paramSet, "squeezeRatio", time, params.squeezeRatio));
  params.axisWarp = static_cast<float>(getDoubleParamAtTime(paramSet, "axisWarp", time, params.axisWarp));
  params.centerProtection =
      static_cast<float>(getDoubleParamAtTime(paramSet, "centerProtection", time, params.centerProtection));
  params.edgeCompressionStart =
      static_cast<float>(getDoubleParamAtTime(paramSet, "edgeCompressionStart", time, params.edgeCompressionStart));
  params.horizontalFovBoost =
      static_cast<float>(getDoubleParamAtTime(paramSet, "horizontalFovBoost", time, params.horizontalFovBoost));
  params.virtualFocalLength =
      static_cast<float>(getDoubleParamAtTime(paramSet, "virtualFocalLength", time, params.virtualFocalLength));
  params.breathingScale = static_cast<float>(getDoubleParamAtTime(paramSet, "breathingScale", time, params.breathingScale));

  params.bokehStretch = static_cast<float>(getDoubleParamAtTime(paramSet, "bokehStretch", time, params.bokehStretch));
  params.bokehRotation = static_cast<float>(getDoubleParamAtTime(paramSet, "bokehRotation", time, params.bokehRotation));
  params.bokehEdgeFalloff =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bokehEdgeFalloff", time, params.bokehEdgeFalloff));
  params.bokehStretchScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bokehStretchScale", time, params.bokehStretchScale));
  params.enableBokeh = getIntParamAtTime(paramSet, "enableBokeh", time, params.enableBokeh);
  params.bokehAmount = static_cast<float>(getDoubleParamAtTime(paramSet, "bokehAmount", time, params.bokehAmount));
  params.focusWidth = static_cast<float>(getDoubleParamAtTime(paramSet, "focusWidth", time, params.focusWidth));
  params.focusFalloff = static_cast<float>(getDoubleParamAtTime(paramSet, "focusFalloff", time, params.focusFalloff));
  params.maxBokehRadius =
      static_cast<float>(getDoubleParamAtTime(paramSet, "maxBokehRadius", time, params.maxBokehRadius));
  params.nearBlurAmount =
      static_cast<float>(getDoubleParamAtTime(paramSet, "nearBlurAmount", time, params.nearBlurAmount));
  params.farBlurAmount =
      static_cast<float>(getDoubleParamAtTime(paramSet, "farBlurAmount", time, params.farBlurAmount));
  params.ovalRatio = static_cast<float>(getDoubleParamAtTime(paramSet, "ovalRatio", time, params.ovalRatio));
  params.ovalOrientation = getIntParamAtTime(paramSet, "ovalOrientation", time, params.ovalOrientation);
  params.ovalAngle = static_cast<float>(getDoubleParamAtTime(paramSet, "ovalAngle", time, params.ovalAngle));
  params.invertDepth = getIntParamAtTime(paramSet, "invertDepth", time, params.invertDepth);
  params.depthBlackPoint =
      static_cast<float>(getDoubleParamAtTime(paramSet, "depthBlackPoint", time, params.depthBlackPoint));
  params.depthWhitePoint =
      static_cast<float>(getDoubleParamAtTime(paramSet, "depthWhitePoint", time, params.depthWhitePoint));
  params.depthGamma = static_cast<float>(getDoubleParamAtTime(paramSet, "depthGamma", time, params.depthGamma));
  params.depthSmoothRadius = getIntParamAtTime(paramSet, "depthSmoothRadius", time, params.depthSmoothRadius);
  params.depthEdgeProtect =
      static_cast<float>(getDoubleParamAtTime(paramSet, "depthEdgeProtect", time, params.depthEdgeProtect));
  params.foregroundEdgeProtect = static_cast<float>(
      getDoubleParamAtTime(paramSet, "foregroundEdgeProtect", time, params.foregroundEdgeProtect));
  params.backgroundEdgeProtect = static_cast<float>(
      getDoubleParamAtTime(paramSet, "backgroundEdgeProtect", time, params.backgroundEdgeProtect));
  params.occlusionThreshold =
      static_cast<float>(getDoubleParamAtTime(paramSet, "occlusionThreshold", time, params.occlusionThreshold));
  params.highlightBokehEnable =
      getIntParamAtTime(paramSet, "highlightBokehEnable", time, params.highlightBokehEnable);
  params.highlightThreshold =
      static_cast<float>(getDoubleParamAtTime(paramSet, "highlightThreshold", time, params.highlightThreshold));
  params.highlightSoftness =
      static_cast<float>(getDoubleParamAtTime(paramSet, "highlightSoftness", time, params.highlightSoftness));
  params.highlightGain =
      static_cast<float>(getDoubleParamAtTime(paramSet, "highlightGain", time, params.highlightGain));
  params.highlightRadiusMultiplier = static_cast<float>(
      getDoubleParamAtTime(paramSet, "highlightRadiusMultiplier", time, params.highlightRadiusMultiplier));
  params.highlightSaturation =
      static_cast<float>(getDoubleParamAtTime(paramSet, "highlightSaturation", time, params.highlightSaturation));
  params.highlightRolloff =
      static_cast<float>(getDoubleParamAtTime(paramSet, "highlightRolloff", time, params.highlightRolloff));
  params.apertureSoftness =
      static_cast<float>(getDoubleParamAtTime(paramSet, "apertureSoftness", time, params.apertureSoftness));
  params.rimBrightness =
      static_cast<float>(getDoubleParamAtTime(paramSet, "rimBrightness", time, params.rimBrightness));
  params.centreDensity =
      static_cast<float>(getDoubleParamAtTime(paramSet, "centreDensity", time, params.centreDensity));
  params.bokehCAEnable = getIntParamAtTime(paramSet, "bokehCAEnable", time, params.bokehCAEnable);
  params.bokehCAAmount =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bokehCAAmount", time, params.bokehCAAmount));
  params.catEyeAmount = static_cast<float>(getDoubleParamAtTime(paramSet, "catEyeAmount", time, params.catEyeAmount));
  params.catEyeStart = static_cast<float>(getDoubleParamAtTime(paramSet, "catEyeStart", time, params.catEyeStart));
  params.catEyeCompression =
      static_cast<float>(getDoubleParamAtTime(paramSet, "catEyeCompression", time, params.catEyeCompression));
  params.catEyeShift = static_cast<float>(getDoubleParamAtTime(paramSet, "catEyeShift", time, params.catEyeShift));
  params.bloomPixelScale = static_cast<float>(getDoubleParamAtTime(paramSet, "bloomPixelScale", time, params.bloomPixelScale));
  params.bloomThresholdScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bloomThresholdScale", time, params.bloomThresholdScale));
  params.bloomRings = getIntParamAtTime(paramSet, "bloomRings", time, params.bloomRings);
  params.bloomSamplesPerRing = getIntParamAtTime(paramSet, "bloomSamplesPerRing", time, params.bloomSamplesPerRing);
  params.bloomEdgeKeepScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bloomEdgeKeepScale", time, params.bloomEdgeKeepScale));
  params.bloomVeilScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bloomVeilScale", time, params.bloomVeilScale));
  params.bloomCreamScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bloomCreamScale", time, params.bloomCreamScale));

  params.flareIntensity =
      static_cast<float>(getDoubleParamAtTime(paramSet, "flareIntensity", time, params.flareIntensity));
  params.flareLength = static_cast<float>(getDoubleParamAtTime(paramSet, "flareLength", time, params.flareLength));
  params.flareColour = getRGBParamAtTime(paramSet, "flareColour", time, params.flareColour);
  params.flareThreshold = static_cast<float>(getDoubleParamAtTime(paramSet, "flareThreshold", time, params.flareThreshold));
  params.flareAngle = static_cast<float>(getDoubleParamAtTime(paramSet, "flareAngle", time, params.flareAngle));
  params.flareStepDensity =
      static_cast<float>(getDoubleParamAtTime(paramSet, "flareStepDensity", time, params.flareStepDensity));
  params.flareSpanScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "flareSpanScale", time, params.flareSpanScale));
  params.flareFalloff = static_cast<float>(getDoubleParamAtTime(paramSet, "flareFalloff", time, params.flareFalloff));

  params.veil = static_cast<float>(getDoubleParamAtTime(paramSet, "veil", time, params.veil));
  params.bloomRadius = static_cast<float>(getDoubleParamAtTime(paramSet, "bloomRadius", time, params.bloomRadius));
  params.highlightCream =
      static_cast<float>(getDoubleParamAtTime(paramSet, "highlightCream", time, params.highlightCream));
  params.blackLiftProtection =
      static_cast<float>(getDoubleParamAtTime(paramSet, "blackLiftProtection", time, params.blackLiftProtection));

  params.ghostCount = getIntParamAtTime(paramSet, "ghostCount", time, params.ghostCount);
  params.ghostSpread = static_cast<float>(getDoubleParamAtTime(paramSet, "ghostSpread", time, params.ghostSpread));
  params.ghostTint = getRGBParamAtTime(paramSet, "ghostTint", time, params.ghostTint);
  params.ghostIntensity = static_cast<float>(getDoubleParamAtTime(paramSet, "ghostIntensity", time, params.ghostIntensity));
  params.coatingStyle = getIntParamAtTime(paramSet, "coatingStyle", time, params.coatingStyle);
  params.coatingWarmResponse =
      static_cast<float>(getDoubleParamAtTime(paramSet, "coatingWarmResponse", time, params.coatingWarmResponse));
  params.coatingCoolResponse =
      static_cast<float>(getDoubleParamAtTime(paramSet, "coatingCoolResponse", time, params.coatingCoolResponse));

  params.edgeBlur = static_cast<float>(getDoubleParamAtTime(paramSet, "edgeBlur", time, params.edgeBlur));
  params.tangentialSmear =
      static_cast<float>(getDoubleParamAtTime(paramSet, "tangentialSmear", time, params.tangentialSmear));
  params.radialFalloff = static_cast<float>(getDoubleParamAtTime(paramSet, "radialFalloff", time, params.radialFalloff));
  params.edgeBlurPixels = static_cast<float>(getDoubleParamAtTime(paramSet, "edgeBlurPixels", time, params.edgeBlurPixels));
  params.fieldCurvaturePixels =
      static_cast<float>(getDoubleParamAtTime(paramSet, "fieldCurvaturePixels", time, params.fieldCurvaturePixels));
  params.smearPixels = static_cast<float>(getDoubleParamAtTime(paramSet, "smearPixels", time, params.smearPixels));

  params.barrel = static_cast<float>(getDoubleParamAtTime(paramSet, "barrel", time, params.barrel));
  params.mustache = static_cast<float>(getDoubleParamAtTime(paramSet, "mustache", time, params.mustache));
  params.verticalCompensation =
      static_cast<float>(getDoubleParamAtTime(paramSet, "verticalCompensation", time, params.verticalCompensation));
  params.verticalCompensationScale = static_cast<float>(
      getDoubleParamAtTime(paramSet, "verticalCompensationScale", time, params.verticalCompensationScale));

  params.closeFocusMumps =
      static_cast<float>(getDoubleParamAtTime(paramSet, "closeFocusMumps", time, params.closeFocusMumps));
  params.faceWidthCompensation = static_cast<float>(
      getDoubleParamAtTime(paramSet, "faceWidthCompensation", time, params.faceWidthCompensation));
  params.enableDepthMap = getIntParamAtTime(paramSet, "enableDepthMap", time, params.enableDepthMap);
  params.focusDistance = static_cast<float>(getDoubleParamAtTime(paramSet, "focusDistance", time, params.focusDistance));
  params.breathingAmount =
      static_cast<float>(getDoubleParamAtTime(paramSet, "breathingAmount", time, params.breathingAmount));
  params.mumpsScale = static_cast<float>(getDoubleParamAtTime(paramSet, "mumpsScale", time, params.mumpsScale));

  params.lateralCA = static_cast<float>(getDoubleParamAtTime(paramSet, "lateralCA", time, params.lateralCA));
  params.longitudinalCA =
      static_cast<float>(getDoubleParamAtTime(paramSet, "longitudinalCA", time, params.longitudinalCA));
  params.edgeOnlyCA = static_cast<float>(getIntParamAtTime(paramSet, "edgeOnlyCA", time, static_cast<int>(params.edgeOnlyCA)));
  params.lateralCAPixelScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "lateralCAPixelScale", time, params.lateralCAPixelScale));

  params.ovalVignette = static_cast<float>(getDoubleParamAtTime(paramSet, "ovalVignette", time, params.ovalVignette));
  params.vignetteAsymmetry =
      static_cast<float>(getDoubleParamAtTime(paramSet, "vignetteAsymmetry", time, params.vignetteAsymmetry));
  params.cornerBias = static_cast<float>(getDoubleParamAtTime(paramSet, "cornerBias", time, params.cornerBias));
  params.ovalVignetteScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "ovalVignetteScale", time, params.ovalVignetteScale));
  params.vignetteAsymmetryScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "vignetteAsymmetryScale", time, params.vignetteAsymmetryScale));

  params.horizontalSmear =
      static_cast<float>(getDoubleParamAtTime(paramSet, "horizontalSmear", time, params.horizontalSmear));
  params.verticalSharpness =
      static_cast<float>(getDoubleParamAtTime(paramSet, "verticalSharpness", time, params.verticalSharpness));
  params.fieldCurvature =
      static_cast<float>(getDoubleParamAtTime(paramSet, "fieldCurvature", time, params.fieldCurvature));

  params.catEyeStrength =
      static_cast<float>(getDoubleParamAtTime(paramSet, "catEyeStrength", time, params.catEyeStrength));
  params.bokehVignette =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bokehVignette", time, params.bokehVignette));
  params.edgeCompression =
      static_cast<float>(getDoubleParamAtTime(paramSet, "edgeCompression", time, params.edgeCompression));
  params.catEyeDimScale = static_cast<float>(getDoubleParamAtTime(paramSet, "catEyeDimScale", time, params.catEyeDimScale));
  params.bokehVignetteDimScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "bokehVignetteDimScale", time, params.bokehVignetteDimScale));
  params.edgeCompressionScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "edgeCompressionScale", time, params.edgeCompressionScale));
  params.centerVeilScale =
      static_cast<float>(getDoubleParamAtTime(paramSet, "centerVeilScale", time, params.centerVeilScale));
    params.enableHighlightEffects =
      getIntParamAtTime(paramSet, "enableHighlightEffects", time, params.enableHighlightEffects);
    params.enableEdgeEffects =
      getIntParamAtTime(paramSet, "enableEdgeEffects", time, params.enableEdgeEffects);
    params.enableAdditionalBackgroundBlur = getIntParamAtTime(paramSet,
                                "enableAdditionalBackgroundBlur",
                                time,
                                params.enableAdditionalBackgroundBlur);

  params.guidesEnabled = getIntParamAtTime(paramSet, "guidesEnabled", time, params.guidesEnabled);
  params.outputAspect = getIntParamAtTime(paramSet, "outputAspect", time, params.outputAspect);
  params.customOutputAspect =
      static_cast<float>(getDoubleParamAtTime(paramSet, "customOutputAspect", time, params.customOutputAspect));
  params.safeArea = static_cast<float>(getDoubleParamAtTime(paramSet, "safeArea", time, params.safeArea));
  params.letterboxPreview = getIntParamAtTime(paramSet, "letterboxPreview", time, params.letterboxPreview);
  params.letterboxOpacity =
      static_cast<float>(getDoubleParamAtTime(paramSet, "letterboxOpacity", time, params.letterboxOpacity));
  params.guideAspectStrength =
      static_cast<float>(getDoubleParamAtTime(paramSet, "guideAspectStrength", time, params.guideAspectStrength));
  params.guideSafeStrength =
      static_cast<float>(getDoubleParamAtTime(paramSet, "guideSafeStrength", time, params.guideSafeStrength));
  params.autoEdgeCrop = getIntParamAtTime(paramSet, "autoEdgeCrop", time, params.autoEdgeCrop);

  return normalizeRenderParams(params);
}

const char *processingBackendName(int backend) {
  switch (backend) {
  case kBackendCpu:
    return "cpu";
  case kBackendAuto:
    return "auto";
  case kBackendMetalExperimental:
    return "metal_experimental";
  default:
    return "unknown";
  }
}

} // namespace rimell
