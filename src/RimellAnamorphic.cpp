#include "Constants.h"
#include "Describe.h"
#include "Diagnostics.h"
#include "HostSuites.h"
#include "MetalRender.h"
#include "ParameterLogic.h"
#include "Parameters.h"
#include "Render.h"
#include "SettingsExport.h"

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"

#include <cstring>
#include <exception>

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__)
#define RIMELL_EXPORT __attribute__((visibility("default")))
#elif defined(_WIN32)
#define RIMELL_EXPORT OfxExport
#else
#error Unsupported platform
#endif

namespace rimell {
namespace {

OfxParamHandle getParamHandle(OfxParamSetHandle paramSet, const char *name) {
  if (!paramSet || !name || !gParameterSuite) {
    return nullptr;
  }

  OfxParamHandle handle = nullptr;
  return gParameterSuite->paramGetHandle(paramSet, name, &handle, nullptr) == kOfxStatOK ? handle : nullptr;
}

void setIntParam(OfxParamSetHandle paramSet, const char *name, int value) {
  if (OfxParamHandle handle = getParamHandle(paramSet, name)) {
    gParameterSuite->paramSetValue(handle, value);
  }
}

void setDoubleParam(OfxParamSetHandle paramSet, const char *name, float value) {
  if (OfxParamHandle handle = getParamHandle(paramSet, name)) {
    gParameterSuite->paramSetValue(handle, static_cast<double>(value));
  }
}

void setRGBParam(OfxParamSetHandle paramSet, const char *name, Vec3 value) {
  if (OfxParamHandle handle = getParamHandle(paramSet, name)) {
    gParameterSuite->paramSetValue(handle,
                                   static_cast<double>(value.r),
                                   static_cast<double>(value.g),
                                   static_cast<double>(value.b));
  }
}

void writePresetParams(OfxParamSetHandle paramSet, const RenderParams &params) {
  setDoubleParam(paramSet, "mix", params.mix);
  setIntParam(paramSet, "inputMode", params.inputMode);
  setIntParam(paramSet, "squeezeMode", params.squeezeMode);
  setDoubleParam(paramSet, "anamorphicTransfer", params.anamorphicTransfer);
  setIntParam(paramSet, "lensIdentity", params.lensIdentity);
  setDoubleParam(paramSet, "squeezeRatio", params.squeezeRatio);
  setDoubleParam(paramSet, "axisWarp", params.axisWarp);
  setDoubleParam(paramSet, "centerProtection", params.centerProtection);
  setDoubleParam(paramSet, "edgeCompressionStart", params.edgeCompressionStart);
  setDoubleParam(paramSet, "horizontalFovBoost", params.horizontalFovBoost);
  setDoubleParam(paramSet, "virtualFocalLength", params.virtualFocalLength);
  setDoubleParam(paramSet, "breathingScale", params.breathingScale);

  setDoubleParam(paramSet, "bokehStretch", params.bokehStretch);
  setDoubleParam(paramSet, "bokehRotation", params.bokehRotation);
  setDoubleParam(paramSet, "bokehEdgeFalloff", params.bokehEdgeFalloff);
  setDoubleParam(paramSet, "bokehStretchScale", params.bokehStretchScale);
  setDoubleParam(paramSet, "bloomPixelScale", params.bloomPixelScale);
  setDoubleParam(paramSet, "bloomThresholdScale", params.bloomThresholdScale);
  setIntParam(paramSet, "bloomRings", params.bloomRings);
  setIntParam(paramSet, "bloomSamplesPerRing", params.bloomSamplesPerRing);
  setDoubleParam(paramSet, "bloomEdgeKeepScale", params.bloomEdgeKeepScale);
  setDoubleParam(paramSet, "bloomVeilScale", params.bloomVeilScale);
  setDoubleParam(paramSet, "bloomCreamScale", params.bloomCreamScale);

  setDoubleParam(paramSet, "flareIntensity", params.flareIntensity);
  setDoubleParam(paramSet, "flareLength", params.flareLength);
  setRGBParam(paramSet, "flareColour", params.flareColour);
  setDoubleParam(paramSet, "flareThreshold", params.flareThreshold);
  setDoubleParam(paramSet, "flareAngle", params.flareAngle);
  setDoubleParam(paramSet, "flareStepDensity", params.flareStepDensity);
  setDoubleParam(paramSet, "flareSpanScale", params.flareSpanScale);
  setDoubleParam(paramSet, "flareFalloff", params.flareFalloff);

  setDoubleParam(paramSet, "veil", params.veil);
  setDoubleParam(paramSet, "bloomRadius", params.bloomRadius);
  setDoubleParam(paramSet, "highlightCream", params.highlightCream);
  setDoubleParam(paramSet, "blackLiftProtection", params.blackLiftProtection);

  setIntParam(paramSet, "ghostCount", params.ghostCount);
  setDoubleParam(paramSet, "ghostSpread", params.ghostSpread);
  setRGBParam(paramSet, "ghostTint", params.ghostTint);
  setDoubleParam(paramSet, "ghostIntensity", params.ghostIntensity);
  setIntParam(paramSet, "coatingStyle", params.coatingStyle);
  setDoubleParam(paramSet, "coatingWarmResponse", params.coatingWarmResponse);
  setDoubleParam(paramSet, "coatingCoolResponse", params.coatingCoolResponse);

  setDoubleParam(paramSet, "edgeBlur", params.edgeBlur);
  setDoubleParam(paramSet, "tangentialSmear", params.tangentialSmear);
  setDoubleParam(paramSet, "radialFalloff", params.radialFalloff);
  setDoubleParam(paramSet, "edgeBlurPixels", params.edgeBlurPixels);
  setDoubleParam(paramSet, "fieldCurvaturePixels", params.fieldCurvaturePixels);
  setDoubleParam(paramSet, "smearPixels", params.smearPixels);

  setDoubleParam(paramSet, "barrel", params.barrel);
  setDoubleParam(paramSet, "mustache", params.mustache);
  setDoubleParam(paramSet, "verticalCompensation", params.verticalCompensation);
  setDoubleParam(paramSet, "verticalCompensationScale", params.verticalCompensationScale);

  setDoubleParam(paramSet, "closeFocusMumps", params.closeFocusMumps);
  setDoubleParam(paramSet, "faceWidthCompensation", params.faceWidthCompensation);
  setIntParam(paramSet, "enableDepthMap", params.enableDepthMap);
  setDoubleParam(paramSet, "focusDistance", params.focusDistance);
  setDoubleParam(paramSet, "breathingAmount", params.breathingAmount);
  setDoubleParam(paramSet, "mumpsScale", params.mumpsScale);

  setDoubleParam(paramSet, "lateralCA", params.lateralCA);
  setDoubleParam(paramSet, "longitudinalCA", params.longitudinalCA);
  setIntParam(paramSet, "edgeOnlyCA", params.edgeOnlyCA > 0.5f ? 1 : 0);
  setDoubleParam(paramSet, "lateralCAPixelScale", params.lateralCAPixelScale);

  setDoubleParam(paramSet, "ovalVignette", params.ovalVignette);
  setDoubleParam(paramSet, "vignetteAsymmetry", params.vignetteAsymmetry);
  setDoubleParam(paramSet, "cornerBias", params.cornerBias);
  setDoubleParam(paramSet, "ovalVignetteScale", params.ovalVignetteScale);
  setDoubleParam(paramSet, "vignetteAsymmetryScale", params.vignetteAsymmetryScale);

  setDoubleParam(paramSet, "horizontalSmear", params.horizontalSmear);
  setDoubleParam(paramSet, "verticalSharpness", params.verticalSharpness);
  setDoubleParam(paramSet, "fieldCurvature", params.fieldCurvature);

  setDoubleParam(paramSet, "catEyeStrength", params.catEyeStrength);
  setDoubleParam(paramSet, "bokehVignette", params.bokehVignette);
  setDoubleParam(paramSet, "edgeCompression", params.edgeCompression);
  setDoubleParam(paramSet, "catEyeDimScale", params.catEyeDimScale);
  setDoubleParam(paramSet, "bokehVignetteDimScale", params.bokehVignetteDimScale);
  setDoubleParam(paramSet, "edgeCompressionScale", params.edgeCompressionScale);
  setDoubleParam(paramSet, "centerVeilScale", params.centerVeilScale);
  setIntParam(paramSet, "enableHighlightEffects", params.enableHighlightEffects);
  setIntParam(paramSet, "enableEdgeEffects", params.enableEdgeEffects);
  setIntParam(paramSet, "enableAdditionalBackgroundBlur", params.enableAdditionalBackgroundBlur);

  setIntParam(paramSet, "guidesEnabled", params.guidesEnabled);
  setIntParam(paramSet, "outputAspect", params.outputAspect);
  setDoubleParam(paramSet, "customOutputAspect", params.customOutputAspect);
  setDoubleParam(paramSet, "safeArea", params.safeArea);
  setIntParam(paramSet, "letterboxPreview", params.letterboxPreview);
  setDoubleParam(paramSet, "letterboxOpacity", params.letterboxOpacity);
  setDoubleParam(paramSet, "guideAspectStrength", params.guideAspectStrength);
  setDoubleParam(paramSet, "guideSafeStrength", params.guideSafeStrength);
  setIntParam(paramSet, "autoEdgeCrop", params.autoEdgeCrop);
}

bool stringEquals(const char *lhs, const char *rhs) {
  return lhs && rhs && std::strcmp(lhs, rhs) == 0;
}

bool isUserEditedChange(OfxPropertySetHandle inArgs) {
  char *reason = nullptr;
  return inArgs && gPropertySuite &&
         gPropertySuite->propGetString(inArgs, kOfxPropChangeReason, 0, &reason) == kOfxStatOK &&
         stringEquals(reason, kOfxChangeUserEdited);
}

OfxStatus applyPresetChange(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs) {
  if (!effect || !inArgs || !gEffectSuite || !gPropertySuite || !gParameterSuite) {
    return kOfxStatReplyDefault;
  }

  char *changedName = nullptr;
  if (gPropertySuite->propGetString(inArgs, kOfxPropName, 0, &changedName) != kOfxStatOK ||
      !changedName || std::strcmp(changedName, "lookPreset") != 0) {
    return kOfxStatOK;
  }

  OfxParamSetHandle paramSet = nullptr;
  if (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK || !paramSet) {
    return kOfxStatErrBadHandle;
  }

  OfxParamHandle presetHandle = getParamHandle(paramSet, "lookPreset");
  if (!presetHandle) {
    return kOfxStatErrBadHandle;
  }

  int preset = kLookPresetManual;
  if (gParameterSuite->paramGetValue(presetHandle, &preset) != kOfxStatOK ||
      preset == kLookPresetManual) {
    return kOfxStatOK;
  }

  RenderParams params;
  params.lookPreset = preset;
  params = clampRenderParams(applyLookPreset(params));

  if (gParameterSuite->paramEditBegin) {
    gParameterSuite->paramEditBegin(paramSet, "Apply Rimell look preset");
  }
  writePresetParams(paramSet, params);
  if (gParameterSuite->paramEditEnd) {
    gParameterSuite->paramEditEnd(paramSet);
  }

  return kOfxStatOK;
}

OfxStatus clearPresetOnUserEdit(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs) {
  if (!effect || !inArgs || !gEffectSuite || !gPropertySuite || !gParameterSuite) {
    return kOfxStatReplyDefault;
  }

  char *changedName = nullptr;
  if (gPropertySuite->propGetString(inArgs, kOfxPropName, 0, &changedName) != kOfxStatOK ||
      !changedName || !isUserEditedChange(inArgs)) {
    return kOfxStatOK;
  }

  if (stringEquals(changedName, "lookPreset")) {
    return applyPresetChange(effect, inArgs);
  }

  OfxParamSetHandle paramSet = nullptr;
  if (gEffectSuite->getParamSet(effect, &paramSet) != kOfxStatOK || !paramSet) {
    return kOfxStatErrBadHandle;
  }

  OfxParamHandle presetHandle = getParamHandle(paramSet, "lookPreset");
  if (!presetHandle) {
    return kOfxStatErrBadHandle;
  }

  int preset = kLookPresetManual;
  if (gParameterSuite->paramGetValue(presetHandle, &preset) != kOfxStatOK || preset == kLookPresetManual) {
    return kOfxStatOK;
  }

  if (gParameterSuite->paramEditBegin) {
    gParameterSuite->paramEditBegin(paramSet, "Clear Rimell look preset");
  }
  gParameterSuite->paramSetValue(presetHandle, static_cast<int>(kLookPresetManual));
  if (gParameterSuite->paramEditEnd) {
    gParameterSuite->paramEditEnd(paramSet);
  }

  return kOfxStatOK;
}

OfxStatus onLoad() {
  if (!gHost) {
    logMessage(LogLevel::Error, "plugin.load", "host is null");
    return kOfxStatErrMissingHostFeature;
  }

  gEffectSuite = const_cast<OfxImageEffectSuiteV1 *>(
      static_cast<const OfxImageEffectSuiteV1 *>(gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1)));
  gPropertySuite = const_cast<OfxPropertySuiteV1 *>(
      static_cast<const OfxPropertySuiteV1 *>(gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1)));
  gParameterSuite = const_cast<OfxParameterSuiteV1 *>(
      static_cast<const OfxParameterSuiteV1 *>(gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1)));

  if (!gEffectSuite || !gPropertySuite || !gParameterSuite) {
    logMessage(LogLevel::Error, "plugin.load", "required suites missing");
    return kOfxStatErrMissingHostFeature;
  }

  logMessage(LogLevel::Info, "plugin.load", "suites loaded successfully");

  return kOfxStatOK;
}

OfxStatus pluginMain(const char *action, const void *handle, OfxPropertySetHandle inArgs,
                     OfxPropertySetHandle outArgs) {
  const char *actionName = action ? action : "(null)";
  ScopedLogTimer actionTimer(LogLevel::Debug, "plugin.main", actionName);
  actionTimer.setResult("in_progress");
  const char *stage = "dispatch";

  try {
    auto effect = static_cast<OfxImageEffectHandle>(const_cast<void *>(handle));
    logPrintf(LogLevel::Trace, "plugin.main", "action=%s", actionName);
    if (!action) {
      logMessage(LogLevel::Error, "plugin.main", "received null action");
      actionTimer.setResult(ofxStatusToString(kOfxStatReplyDefault));
      return kOfxStatReplyDefault;
    }

    if (std::strcmp(action, kOfxActionLoad) == 0) {
      stage = "load";
      const OfxStatus status = onLoad();
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxActionUnload) == 0) {
      stage = "unload";
      clearMetalPipelineCaches();
      gEffectSuite = nullptr;
      gPropertySuite = nullptr;
      gParameterSuite = nullptr;
      logMessage(LogLevel::Info, "plugin.unload", "suites cleared");
      actionTimer.setResult(ofxStatusToString(kOfxStatOK));
      return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
      stage = "describe";
      const OfxStatus status = describe(effect);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
      stage = "describe_in_context";
      const OfxStatus status = describeInContext(effect);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0 ||
        std::strcmp(action, kOfxActionDestroyInstance) == 0 ||
        std::strcmp(action, kOfxImageEffectActionBeginSequenceRender) == 0 ||
        std::strcmp(action, kOfxImageEffectActionEndSequenceRender) == 0) {
      stage = "lifecycle_noop";
      actionTimer.setResult(ofxStatusToString(kOfxStatOK));
      return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionInstanceChanged) == 0) {
      stage = "instance_changed";
      char *changedName = nullptr;
      if (inArgs && gPropertySuite && gPropertySuite->propGetString(inArgs, kOfxPropName, 0, &changedName) == kOfxStatOK && changedName) {
        if (std::strcmp(changedName, "generateJson") == 0) {
          OfxParamSetHandle paramSet = nullptr;
          if (gEffectSuite->getParamSet(effect, &paramSet) == kOfxStatOK) {
            RenderParams params = readParams(effect);
            std::string jsonStr = generateSettingsJson(params);
            setStringParam(paramSet, "settingsJson", jsonStr);
          }
          actionTimer.setResult("OK");
          return kOfxStatOK;
        }
      }
      const OfxStatus status = isUserEditedChange(inArgs) ? clearPresetOnUserEdit(effect, inArgs)
                                                         : applyPresetChange(effect, inArgs);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxImageEffectActionIsIdentity) == 0) {
      stage = "is_identity";
      const OfxStatus status = isIdentity(effect, inArgs, outArgs);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
      stage = "get_rod";
      const OfxStatus status = getRegionOfDefinition(effect, inArgs, outArgs);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0) {
      stage = "get_roi";
      const OfxStatus status = getRegionsOfInterest(effect, inArgs, outArgs);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
      stage = "get_clip_preferences";
      const OfxStatus status = getClipPreferences(effect, outArgs);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
    if (std::strcmp(action, kOfxImageEffectActionRender) == 0) {
      stage = "render";
      const OfxStatus status = render(effect, inArgs);
      actionTimer.setResult(ofxStatusToString(status));
      return status;
    }
  } catch (const std::bad_alloc &) {
    logPrintf(LogLevel::Error,
              "plugin.main",
              "bad_alloc stage=%s action=%s",
              stage,
              actionName);
    actionTimer.setResult(ofxStatusToString(kOfxStatErrMemory));
    return kOfxStatErrMemory;
  } catch (const std::exception &ex) {
    logPrintf(LogLevel::Error,
              "plugin.main",
              "std::exception stage=%s action=%s what=%s",
              stage,
              actionName,
              ex.what());
    actionTimer.setResult(ofxStatusToString(kOfxStatErrUnknown));
    return kOfxStatErrUnknown;
  } catch (...) {
    logPrintf(LogLevel::Error,
              "plugin.main",
              "unknown exception stage=%s action=%s",
              stage,
              actionName);
    actionTimer.setResult(ofxStatusToString(kOfxStatErrUnknown));
    return kOfxStatErrUnknown;
  }

  actionTimer.setResult(ofxStatusToString(kOfxStatReplyDefault));
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
} // namespace rimell

extern "C" {

RIMELL_EXPORT OfxPlugin *OfxGetPlugin(int nth) {
  return nth == 0 ? &rimell::plugin : nullptr;
}

RIMELL_EXPORT int OfxGetNumberOfPlugins() {
  return 1;
}
}
