#include "ParameterLogic.h"

#include <algorithm>

namespace rimell {
namespace {

template <typename T>
T clampValue(T value, T minValue, T maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

void applySubtleModern(RenderParams &params) {
  params.lensIdentity = 1;
  params.squeezeRatio = 1.33f;
  params.anamorphicTransfer = 0.72f;
  params.centerProtection = 0.72f;
  params.edgeCompression = 0.12f;
  params.edgeBlur = 0.035f;
  params.tangentialSmear = 0.02f;
  params.horizontalSmear = 0.02f;
  params.lateralCA = 0.025f;
  params.veil = 0.025f;
  params.bloomRadius = 0.1f;
  params.flareIntensity = 0.05f;
  params.flareLength = 0.35f;
  params.ghostCount = 0;
  params.ovalVignette = 0.04f;
}

void applyClassic2x(RenderParams &params) {
  params.lensIdentity = 2;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.92f;
  params.centerProtection = 0.5f;
  params.edgeCompression = 0.34f;
  params.bokehStretch = 0.42f;
  params.bokehEdgeFalloff = 0.24f;
  params.edgeBlur = 0.11f;
  params.tangentialSmear = 0.11f;
  params.horizontalSmear = 0.08f;
  params.fieldCurvature = 0.08f;
  params.lateralCA = 0.075f;
  params.veil = 0.055f;
  params.bloomRadius = 0.18f;
  params.flareIntensity = 0.14f;
  params.flareLength = 0.62f;
  params.ghostCount = 2;
  params.ghostIntensity = 0.16f;
  params.ovalVignette = 0.11f;
  params.closeFocusMumps = 0.18f;
}

void applyNightFlare(RenderParams &params) {
  params.lensIdentity = 3;
  params.squeezeRatio = 1.8f;
  params.anamorphicTransfer = 0.82f;
  params.centerProtection = 0.58f;
  params.bokehStretch = 0.48f;
  params.bokehEdgeFalloff = 0.18f;
  params.veil = 0.08f;
  params.bloomRadius = 0.28f;
  params.highlightCream = 0.2f;
  params.blackLiftProtection = 0.78f;
  params.flareIntensity = 0.38f;
  params.flareLength = 0.86f;
  params.flareThreshold = 0.74f;
  params.flareStepDensity = 10.0f;
  params.ghostCount = 3;
  params.ghostSpread = 0.42f;
  params.ghostIntensity = 0.2f;
  params.edgeBlur = 0.07f;
  params.lateralCA = 0.055f;
  params.ovalVignette = 0.08f;
}

void applyGeometryOnly(RenderParams &params) {
  params.lensIdentity = 1;
  params.squeezeRatio = 1.5f;
  params.anamorphicTransfer = 0.8f;
  params.centerProtection = 0.65f;
  params.edgeCompression = 0.2f;
  params.barrel = 0.02f;
  params.mustache = -0.01f;
  params.veil = 0.0f;
  params.bloomRadius = 0.0f;
  params.highlightCream = 0.0f;
  params.flareIntensity = 0.0f;
  params.ghostCount = 0;
  params.edgeBlur = 0.0f;
  params.tangentialSmear = 0.0f;
  params.horizontalSmear = 0.0f;
  params.lateralCA = 0.0f;
  params.longitudinalCA = 0.0f;
  params.ovalVignette = 0.0f;
  params.catEyeStrength = 0.0f;
  params.bokehVignette = 0.0f;
}

void applySoftScope(RenderParams &params) {
  params.lensIdentity = 1;
  params.squeezeRatio = 1.5f;
  params.anamorphicTransfer = 0.64f;
  params.centerProtection = 0.78f;
  params.edgeCompression = 0.08f;
  params.bokehStretch = 0.22f;
  params.bokehEdgeFalloff = 0.18f;
  params.edgeBlur = 0.03f;
  params.tangentialSmear = 0.015f;
  params.horizontalSmear = 0.015f;
  params.lateralCA = 0.02f;
  params.veil = 0.02f;
  params.bloomRadius = 0.14f;
  params.flareIntensity = 0.035f;
  params.flareLength = 0.28f;
  params.ghostCount = 0;
  params.ovalVignette = 0.03f;
}

void applyWarmGlass(RenderParams &params) {
  params.lensIdentity = 3;
  params.squeezeRatio = 1.85f;
  params.anamorphicTransfer = 0.88f;
  params.centerProtection = 0.6f;
  params.edgeCompression = 0.22f;
  params.bokehStretch = 0.38f;
  params.bokehEdgeFalloff = 0.2f;
  params.veil = 0.055f;
  params.bloomRadius = 0.2f;
  params.highlightCream = 0.12f;
  params.blackLiftProtection = 0.72f;
  params.flareIntensity = 0.18f;
  params.flareLength = 0.72f;
  params.flareThreshold = 0.76f;
  params.flareStepDensity = 8.0f;
  params.flareColour = {0.92f, 0.56f, 0.28f};
  params.ghostCount = 2;
  params.ghostSpread = 0.4f;
  params.ghostIntensity = 0.14f;
  params.ghostTint = {0.95f, 0.75f, 0.5f};
  params.coatingStyle = 0;
  params.coatingWarmResponse = 1.5f;
  params.coatingCoolResponse = 0.8f;
  params.edgeBlur = 0.06f;
  params.lateralCA = 0.045f;
  params.ovalVignette = 0.08f;
}

void applyVintageWide(RenderParams &params) {
  params.lensIdentity = 2;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.95f;
  params.centerProtection = 0.46f;
  params.edgeCompression = 0.38f;
  params.bokehStretch = 0.5f;
  params.bokehEdgeFalloff = 0.26f;
  params.edgeBlur = 0.12f;
  params.tangentialSmear = 0.12f;
  params.horizontalSmear = 0.1f;
  params.fieldCurvature = 0.1f;
  params.lateralCA = 0.09f;
  params.veil = 0.075f;
  params.bloomRadius = 0.22f;
  params.flareIntensity = 0.16f;
  params.flareLength = 0.64f;
  params.ghostCount = 3;
  params.ghostSpread = 0.45f;
  params.ghostIntensity = 0.18f;
  params.ovalVignette = 0.14f;
  params.closeFocusMumps = 0.2f;
}

void applyCleanPrime(RenderParams &params) {
  params.lensIdentity = 1;
  params.squeezeRatio = 1.33f;
  params.anamorphicTransfer = 0.84f;
  params.centerProtection = 0.86f;
  params.edgeCompression = 0.04f;
  params.bokehStretch = 0.12f;
  params.bokehEdgeFalloff = 0.12f;
  params.edgeBlur = 0.0f;
  params.tangentialSmear = 0.0f;
  params.horizontalSmear = 0.0f;
  params.lateralCA = 0.01f;
  params.longitudinalCA = 0.0f;
  params.veil = 0.0f;
  params.bloomRadius = 0.06f;
  params.highlightCream = 0.02f;
  params.flareIntensity = 0.02f;
  params.flareLength = 0.18f;
  params.ghostCount = 0;
  params.ovalVignette = 0.02f;
  params.catEyeStrength = 0.0f;
  params.bokehVignette = 0.0f;
}

} // namespace

RenderParams clampRenderParams(RenderParams params) {
  params.mix = clampValue(params.mix, 0.0f, 1.0f);
  params.debugView = clampValue(params.debugView, 0, 3);
  params.renderQuality = clampValue(params.renderQuality, 0, 2);
  params.lookPreset = clampValue(params.lookPreset, static_cast<int>(kLookPresetManual),
                                 static_cast<int>(kLookPresetCleanPrime));
  params.inputMode = clampValue(params.inputMode, 0, 2);
  params.squeezeMode = clampValue(params.squeezeMode, 0, 2);
  params.anamorphicTransfer = clampValue(params.anamorphicTransfer, 0.0f, 1.0f);
  params.lensIdentity = clampValue(params.lensIdentity, 0, 3);
  params.squeezeRatio = clampValue(params.squeezeRatio, 1.0f, 2.0f);
  params.axisWarp = clampValue(params.axisWarp, 0.0f, 1.0f);
  params.centerProtection = clampValue(params.centerProtection, 0.0f, 1.0f);
  params.edgeCompressionStart = clampValue(params.edgeCompressionStart, 0.0f, 1.0f);
  params.bloomRings = clampValue(params.bloomRings, 1, 8);
  params.bloomSamplesPerRing = clampValue(params.bloomSamplesPerRing, 3, 32);
  params.ghostCount = clampValue(params.ghostCount, 0, 8);
  params.coatingStyle = clampValue(params.coatingStyle, 0, 2);
  params.longitudinalCA = clampValue(params.longitudinalCA, 0.0f, 1.0f);
  params.edgeOnlyCA = params.edgeOnlyCA > 0.5f ? 1.0f : 0.0f;
  params.guidesEnabled = params.guidesEnabled != 0 ? 1 : 0;
  params.outputAspect = clampValue(params.outputAspect, 0, 3);
  params.customOutputAspect = std::max(0.1f, params.customOutputAspect);
  params.safeArea = clampValue(params.safeArea, 0.5f, 1.0f);
  params.letterboxPreview = params.letterboxPreview != 0 ? 1 : 0;
  params.letterboxOpacity = clampValue(params.letterboxOpacity, 0.0f, 1.0f);
  params.autoEdgeCrop = params.autoEdgeCrop != 0 ? 1 : 0;
  params.edgeCropScale = std::max(1.0f, params.edgeCropScale);
  return params;
}

RenderParams applyLookPreset(RenderParams params) {
  switch (params.lookPreset) {
  case kLookPresetSubtleModern:
    applySubtleModern(params);
    break;
  case kLookPresetClassic2x:
    applyClassic2x(params);
    break;
  case kLookPresetNightFlare:
    applyNightFlare(params);
    break;
  case kLookPresetGeometryOnly:
    applyGeometryOnly(params);
    break;
  case kLookPresetSoftScope:
    applySoftScope(params);
    break;
  case kLookPresetWarmGlass:
    applyWarmGlass(params);
    break;
  case kLookPresetVintageWide:
    applyVintageWide(params);
    break;
  case kLookPresetCleanPrime:
    applyCleanPrime(params);
    break;
  default:
    break;
  }
  return params;
}

RenderParams normalizeRenderParams(RenderParams params) {
  return clampRenderParams(applyLookPreset(clampRenderParams(params)));
}

float aspectValue(int index, float customOutputAspect) {
  switch (index) {
  case 0:
    return 2.0f;
  case 1:
    return 2.39f;
  case 2:
    return 2.66f;
  case 3:
    return std::max(0.1f, customOutputAspect);
  default:
    return 2.39f;
  }
}

} // namespace rimell
