#include "ParameterDefinition.h"

#include "HostSuites.h"

#include <initializer_list>

namespace rimell {

void addDoubleParam(OfxParamSetHandle paramSet, const char *name, const char *label, double defaultValue,
                    double minValue, double maxValue, double displayMin, double displayMax,
                    const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeDouble, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 0, defaultValue);
  gPropertySuite->propSetDouble(props, kOfxParamPropMin, 0, minValue);
  gPropertySuite->propSetDouble(props, kOfxParamPropMax, 0, maxValue);
  gPropertySuite->propSetDouble(props, kOfxParamPropDisplayMin, 0, displayMin);
  gPropertySuite->propSetDouble(props, kOfxParamPropDisplayMax, 0, displayMax);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addIntParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                 int minValue, int maxValue, const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeInteger, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropDefault, 0, defaultValue);
  gPropertySuite->propSetInt(props, kOfxParamPropMin, 0, minValue);
  gPropertySuite->propSetInt(props, kOfxParamPropMax, 0, maxValue);
  gPropertySuite->propSetInt(props, kOfxParamPropDisplayMin, 0, minValue);
  gPropertySuite->propSetInt(props, kOfxParamPropDisplayMax, 0, maxValue);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addBooleanParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                     const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeBoolean, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropDefault, 0, defaultValue);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addStringParam(OfxParamSetHandle paramSet, const char *name, const char *label, const char *defaultValue,
                    const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeString, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetString(props, kOfxParamPropDefault, 0, defaultValue ? defaultValue : "");
  gPropertySuite->propSetInt(props, kOfxParamPropEnabled, 0, 0);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addStringFilePathParam(OfxParamSetHandle paramSet, const char *name, const char *label,
                            const char *defaultValue, const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeString, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetString(props, kOfxParamPropStringMode, 0, kOfxParamStringIsFilePath);
  gPropertySuite->propSetString(props, kOfxParamPropDefault, 0, defaultValue ? defaultValue : "");
  gPropertySuite->propSetInt(props, kOfxParamPropStringFilePathExists, 0, 0); // Don't require it to exist
  gPropertySuite->propSetInt(props, kOfxParamPropEnabled, 0, 1);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addStringMultiLineParam(OfxParamSetHandle paramSet, const char *name, const char *label,
                             const char *defaultValue, const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeString, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetString(props, kOfxParamPropStringMode, 0, kOfxParamStringIsMultiLine);
  gPropertySuite->propSetString(props, kOfxParamPropDefault, 0, defaultValue ? defaultValue : "");
  gPropertySuite->propSetInt(props, kOfxParamPropEnabled, 0, 1);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addPushButtonParam(OfxParamSetHandle paramSet, const char *name, const char *label,
                        const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypePushButton, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addGroupParam(OfxParamSetHandle paramSet, const char *name, const char *label, bool openByDefault,
                   const char *hint) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeGroup, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropGroupOpen, 0, openByDefault ? 1 : 0);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
}

void addPageParam(OfxParamSetHandle paramSet, const char *name, const char *label) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypePage, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
}

void addChoiceParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                    std::initializer_list<const char *> options, const char *hint,
                    const char *parent) {
  if (!paramSet || !name || !label || options.size() < 2 || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeChoice, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropDefault, 0, defaultValue);
  int index = 0;
  for (const char *option : options) {
    gPropertySuite->propSetString(props, kOfxParamPropChoiceOption, index++, option);
  }
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addRGBParam(OfxParamSetHandle paramSet, const char *name, const char *label, Vec3 defaultValue,
                 const char *hint, const char *parent) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeRGB, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 0, defaultValue.r);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 1, defaultValue.g);
  gPropertySuite->propSetDouble(props, kOfxParamPropDefault, 2, defaultValue.b);
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
  if (parent) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void setParamFlags(OfxParamSetHandle paramSet, const char *name, int secret, int enabled, int persistent,
                   int evaluateOnChange) {
  if (!paramSet || !name || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxParamHandle handle = nullptr;
  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetInt(props, kOfxParamPropSecret, 0, secret);
  gPropertySuite->propSetInt(props, kOfxParamPropEnabled, 0, enabled);
  gPropertySuite->propSetInt(props, kOfxParamPropPersistant, 0, persistent);
  gPropertySuite->propSetInt(props, kOfxParamPropEvaluateOnChange, 0, evaluateOnChange);
}

void setParamParent(OfxParamSetHandle paramSet, const char *name, const char *parent) {
  if (!paramSet || !name || !parent || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxParamHandle handle = nullptr;
  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, name, &handle, &props) == kOfxStatOK && props) {
    gPropertySuite->propSetString(props, kOfxParamPropParent, 0, parent);
  }
}

void addPageChild(OfxParamSetHandle paramSet, const char *pageName, const char *childName) {
  if (!paramSet || !pageName || !childName || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxParamHandle handle = nullptr;
  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramGetHandle(paramSet, pageName, &handle, &props) == kOfxStatOK && props) {
    int childCount = 0;
    if (gPropertySuite->propGetDimension(props, kOfxParamPropPageChild, &childCount) != kOfxStatOK) {
      childCount = 0;
    }
    gPropertySuite->propSetString(props, kOfxParamPropPageChild, childCount, childName);
  }
}

} // namespace rimell
