#include "Constants.h"
#include "Describe.h"
#include "HostSuites.h"
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
                     OfxPropertySetHandle outArgs) {
  try {
    auto effect = static_cast<OfxImageEffectHandle>(const_cast<void *>(handle));

    if (std::strcmp(action, kOfxActionLoad) == 0) {
      return onLoad();
    }
    if (std::strcmp(action, kOfxActionUnload) == 0) {
      gEffectSuite = nullptr;
      gPropertySuite = nullptr;
      gParameterSuite = nullptr;
      return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxActionDescribe) == 0) {
      return describe(effect);
    }
    if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
      return describeInContext(effect);
    }
    if (std::strcmp(action, kOfxActionCreateInstance) == 0 ||
        std::strcmp(action, kOfxActionDestroyInstance) == 0 ||
        std::strcmp(action, kOfxImageEffectActionBeginSequenceRender) == 0 ||
        std::strcmp(action, kOfxImageEffectActionEndSequenceRender) == 0) {
      return kOfxStatOK;
    }
    if (std::strcmp(action, kOfxImageEffectActionIsIdentity) == 0) {
      return isIdentity(effect, inArgs, outArgs);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
      return getRegionOfDefinition(effect, inArgs, outArgs);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0) {
      return getRegionsOfInterest(effect, inArgs, outArgs);
    }
    if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
      return getClipPreferences(effect, outArgs);
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
} // namespace rimell

extern "C" {

RIMELL_EXPORT OfxPlugin *OfxGetPlugin(int nth) {
  return nth == 0 ? &rimell::plugin : nullptr;
}

RIMELL_EXPORT int OfxGetNumberOfPlugins() {
  return 1;
}
}
