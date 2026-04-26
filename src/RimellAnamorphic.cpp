#include "Constants.h"
#include "Describe.h"
#include "Diagnostics.h"
#include "HostSuites.h"
#include "MetalRender.h"
#include "Render.h"

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
