#include "Parameters.h"

#include "HostSuites.h"
#include "ParameterLogic.h"

#include <algorithm>

namespace rimell {

double getDoubleParam(OfxParamSetHandle paramSet, const char *name, double fallback) {
    if (!paramSet || !name || !gParameterSuite) {
        return fallback;
    }

  OfxParamHandle handle = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr) != kOfxStatOK || !handle) {
    return fallback;
  }

  double value = fallback;
    if (gParameterSuite->paramGetValue(handle, &value) != kOfxStatOK) {
        return fallback;
    }
  return value;
}

int getIntParam(OfxParamSetHandle paramSet, const char *name, int fallback) {
    if (!paramSet || !name || !gParameterSuite) {
        return fallback;
    }

  OfxParamHandle handle = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr) != kOfxStatOK || !handle) {
    return fallback;
  }

  int value = fallback;
    if (gParameterSuite->paramGetValue(handle, &value) != kOfxStatOK) {
        return fallback;
    }
  return value;
}

Vec3 getRGBParam(OfxParamSetHandle paramSet, const char *name, Vec3 fallback) {
    if (!paramSet || !name || !gParameterSuite) {
        return fallback;
    }

  OfxParamHandle handle = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr) != kOfxStatOK || !handle) {
    return fallback;
  }

  double r = fallback.r;
  double g = fallback.g;
  double b = fallback.b;
    if (gParameterSuite->paramGetValue(handle, &r, &g, &b) != kOfxStatOK) {
        return fallback;
    }
  return {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};
}

RenderParams readParams(OfxImageEffectHandle effect) {
  OfxParamSetHandle paramSet = nullptr;
    if (!effect || !gEffectSuite || gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK || !paramSet) {
        return normalizeRenderParams(RenderParams{});
    }

  RenderParams params;
  params.mix = static_cast<float>(getDoubleParam(paramSet, "mix", params.mix));
    params.debugView = getIntParam(paramSet, "debugView", params.debugView);
  params.renderQuality = getIntParam(paramSet, "renderQuality", params.renderQuality);
  params.lookPreset = getIntParam(paramSet, "lookPreset", params.lookPreset);
  params.inputMode = getIntParam(paramSet, "inputMode", params.inputMode);
  params.squeezeMode = getIntParam(paramSet, "squeezeMode", params.squeezeMode);
  params.anamorphicTransfer =
      static_cast<float>(getDoubleParam(paramSet, "anamorphicTransfer", params.anamorphicTransfer));
  params.lensIdentity = getIntParam(paramSet, "lensIdentity", params.lensIdentity);
  params.squeezeRatio = static_cast<float>(getDoubleParam(paramSet, "squeezeRatio", params.squeezeRatio));
  params.axisWarp = static_cast<float>(getDoubleParam(paramSet, "axisWarp", params.axisWarp));
  params.centerProtection =
      static_cast<float>(getDoubleParam(paramSet, "centerProtection", params.centerProtection));
  params.edgeCompressionStart =
      static_cast<float>(getDoubleParam(paramSet, "edgeCompressionStart", params.edgeCompressionStart));
  params.horizontalFovBoost =
      static_cast<float>(getDoubleParam(paramSet, "horizontalFovBoost", params.horizontalFovBoost));
  params.virtualFocalLength =
      static_cast<float>(getDoubleParam(paramSet, "virtualFocalLength", params.virtualFocalLength));
  params.breathingScale = static_cast<float>(getDoubleParam(paramSet, "breathingScale", params.breathingScale));

  params.bokehStretch = static_cast<float>(getDoubleParam(paramSet, "bokehStretch", params.bokehStretch));
  params.bokehRotation = static_cast<float>(getDoubleParam(paramSet, "bokehRotation", params.bokehRotation));
  params.bokehEdgeFalloff =
      static_cast<float>(getDoubleParam(paramSet, "bokehEdgeFalloff", params.bokehEdgeFalloff));
  params.bokehStretchScale =
      static_cast<float>(getDoubleParam(paramSet, "bokehStretchScale", params.bokehStretchScale));
  params.bloomPixelScale = static_cast<float>(getDoubleParam(paramSet, "bloomPixelScale", params.bloomPixelScale));
  params.bloomThresholdScale =
      static_cast<float>(getDoubleParam(paramSet, "bloomThresholdScale", params.bloomThresholdScale));
  params.bloomRings = getIntParam(paramSet, "bloomRings", params.bloomRings);
  params.bloomSamplesPerRing = getIntParam(paramSet, "bloomSamplesPerRing", params.bloomSamplesPerRing);
  params.bloomEdgeKeepScale =
      static_cast<float>(getDoubleParam(paramSet, "bloomEdgeKeepScale", params.bloomEdgeKeepScale));
  params.bloomVeilScale =
      static_cast<float>(getDoubleParam(paramSet, "bloomVeilScale", params.bloomVeilScale));
  params.bloomCreamScale =
      static_cast<float>(getDoubleParam(paramSet, "bloomCreamScale", params.bloomCreamScale));

  params.flareIntensity =
      static_cast<float>(getDoubleParam(paramSet, "flareIntensity", params.flareIntensity));
  params.flareLength = static_cast<float>(getDoubleParam(paramSet, "flareLength", params.flareLength));
  params.flareColour = getRGBParam(paramSet, "flareColour", params.flareColour);
  params.flareThreshold = static_cast<float>(getDoubleParam(paramSet, "flareThreshold", params.flareThreshold));
  params.flareAngle = static_cast<float>(getDoubleParam(paramSet, "flareAngle", params.flareAngle));
  params.flareStepDensity =
      static_cast<float>(getDoubleParam(paramSet, "flareStepDensity", params.flareStepDensity));
  params.flareSpanScale =
      static_cast<float>(getDoubleParam(paramSet, "flareSpanScale", params.flareSpanScale));
  params.flareFalloff = static_cast<float>(getDoubleParam(paramSet, "flareFalloff", params.flareFalloff));

  params.veil = static_cast<float>(getDoubleParam(paramSet, "veil", params.veil));
  params.bloomRadius = static_cast<float>(getDoubleParam(paramSet, "bloomRadius", params.bloomRadius));
  params.highlightCream =
      static_cast<float>(getDoubleParam(paramSet, "highlightCream", params.highlightCream));
  params.blackLiftProtection =
      static_cast<float>(getDoubleParam(paramSet, "blackLiftProtection", params.blackLiftProtection));

  params.ghostCount = getIntParam(paramSet, "ghostCount", params.ghostCount);
  params.ghostSpread = static_cast<float>(getDoubleParam(paramSet, "ghostSpread", params.ghostSpread));
  params.ghostTint = getRGBParam(paramSet, "ghostTint", params.ghostTint);
  params.ghostIntensity = static_cast<float>(getDoubleParam(paramSet, "ghostIntensity", params.ghostIntensity));
  params.coatingStyle = getIntParam(paramSet, "coatingStyle", params.coatingStyle);
  params.coatingWarmResponse =
      static_cast<float>(getDoubleParam(paramSet, "coatingWarmResponse", params.coatingWarmResponse));
  params.coatingCoolResponse =
      static_cast<float>(getDoubleParam(paramSet, "coatingCoolResponse", params.coatingCoolResponse));

  params.edgeBlur = static_cast<float>(getDoubleParam(paramSet, "edgeBlur", params.edgeBlur));
  params.tangentialSmear =
      static_cast<float>(getDoubleParam(paramSet, "tangentialSmear", params.tangentialSmear));
  params.radialFalloff = static_cast<float>(getDoubleParam(paramSet, "radialFalloff", params.radialFalloff));
  params.edgeBlurPixels = static_cast<float>(getDoubleParam(paramSet, "edgeBlurPixels", params.edgeBlurPixels));
  params.fieldCurvaturePixels =
      static_cast<float>(getDoubleParam(paramSet, "fieldCurvaturePixels", params.fieldCurvaturePixels));
  params.smearPixels = static_cast<float>(getDoubleParam(paramSet, "smearPixels", params.smearPixels));

  params.barrel = static_cast<float>(getDoubleParam(paramSet, "barrel", params.barrel));
  params.mustache = static_cast<float>(getDoubleParam(paramSet, "mustache", params.mustache));
  params.verticalCompensation =
      static_cast<float>(getDoubleParam(paramSet, "verticalCompensation", params.verticalCompensation));
  params.verticalCompensationScale = static_cast<float>(
      getDoubleParam(paramSet, "verticalCompensationScale", params.verticalCompensationScale));

  params.closeFocusMumps =
      static_cast<float>(getDoubleParam(paramSet, "closeFocusMumps", params.closeFocusMumps));
  params.faceWidthCompensation = static_cast<float>(
      getDoubleParam(paramSet, "faceWidthCompensation", params.faceWidthCompensation));
  params.focusDistance = static_cast<float>(getDoubleParam(paramSet, "focusDistance", params.focusDistance));
  params.breathingAmount =
      static_cast<float>(getDoubleParam(paramSet, "breathingAmount", params.breathingAmount));
  params.mumpsScale = static_cast<float>(getDoubleParam(paramSet, "mumpsScale", params.mumpsScale));

  params.lateralCA = static_cast<float>(getDoubleParam(paramSet, "lateralCA", params.lateralCA));
  params.longitudinalCA =
      static_cast<float>(getDoubleParam(paramSet, "longitudinalCA", params.longitudinalCA));
  params.edgeOnlyCA = static_cast<float>(getIntParam(paramSet, "edgeOnlyCA", static_cast<int>(params.edgeOnlyCA)));
  params.lateralCAPixelScale =
      static_cast<float>(getDoubleParam(paramSet, "lateralCAPixelScale", params.lateralCAPixelScale));

  params.ovalVignette = static_cast<float>(getDoubleParam(paramSet, "ovalVignette", params.ovalVignette));
  params.vignetteAsymmetry =
      static_cast<float>(getDoubleParam(paramSet, "vignetteAsymmetry", params.vignetteAsymmetry));
  params.cornerBias = static_cast<float>(getDoubleParam(paramSet, "cornerBias", params.cornerBias));
  params.ovalVignetteScale =
      static_cast<float>(getDoubleParam(paramSet, "ovalVignetteScale", params.ovalVignetteScale));
  params.vignetteAsymmetryScale =
      static_cast<float>(getDoubleParam(paramSet, "vignetteAsymmetryScale", params.vignetteAsymmetryScale));

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
  params.catEyeDimScale = static_cast<float>(getDoubleParam(paramSet, "catEyeDimScale", params.catEyeDimScale));
  params.bokehVignetteDimScale =
      static_cast<float>(getDoubleParam(paramSet, "bokehVignetteDimScale", params.bokehVignetteDimScale));
  params.edgeCompressionScale =
      static_cast<float>(getDoubleParam(paramSet, "edgeCompressionScale", params.edgeCompressionScale));
  params.centerVeilScale =
      static_cast<float>(getDoubleParam(paramSet, "centerVeilScale", params.centerVeilScale));

  params.guidesEnabled = getIntParam(paramSet, "guidesEnabled", params.guidesEnabled);
  params.outputAspect = getIntParam(paramSet, "outputAspect", params.outputAspect);
  params.customOutputAspect =
      static_cast<float>(getDoubleParam(paramSet, "customOutputAspect", params.customOutputAspect));
  params.safeArea = static_cast<float>(getDoubleParam(paramSet, "safeArea", params.safeArea));
  params.letterboxPreview = getIntParam(paramSet, "letterboxPreview", params.letterboxPreview);
  params.letterboxOpacity =
      static_cast<float>(getDoubleParam(paramSet, "letterboxOpacity", params.letterboxOpacity));
  params.guideAspectStrength =
      static_cast<float>(getDoubleParam(paramSet, "guideAspectStrength", params.guideAspectStrength));
  params.guideSafeStrength =
      static_cast<float>(getDoubleParam(paramSet, "guideSafeStrength", params.guideSafeStrength));
  params.autoEdgeCrop = getIntParam(paramSet, "autoEdgeCrop", params.autoEdgeCrop);

  return normalizeRenderParams(params);
}

} // namespace rimell
