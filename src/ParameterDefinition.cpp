#include "ParameterDefinition.h"

#include "HostSuites.h"

namespace rimell {

void addDoubleParam(OfxParamSetHandle paramSet, const char *name, const char *label, double defaultValue,
                    double minValue, double maxValue, double displayMin, double displayMax,
                    const char *hint) {
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
}

void addIntParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                 int minValue, int maxValue, const char *hint) {
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
}

void addBooleanParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                     const char *hint) {
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
}

void addGroupParam(OfxParamSetHandle paramSet, const char *name, const char *label, int open) {
  if (!paramSet || !name || !label || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeGroup, name, &props) != kOfxStatOK || !props) {
    return;
  }

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, label);
  gPropertySuite->propSetInt(props, kOfxParamPropGroupOpen, 0, open);
}

void addChoiceParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                    const char *option0, const char *option1, const char *option2,
                    const char *option3, const char *option4, const char *hint) {
  if (!paramSet || !name || !label || !option0 || !option1 || !gParameterSuite || !gPropertySuite) {
    return;
  }

  OfxPropertySetHandle props = nullptr;
  if (gParameterSuite->paramDefine(paramSet, kOfxParamTypeChoice, name, &props) != kOfxStatOK || !props) {
    return;
  }

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
  if (option4) {
    gPropertySuite->propSetString(props, kOfxParamPropChoiceOption, 4, option4);
  }
  if (hint) {
    gPropertySuite->propSetString(props, kOfxParamPropHint, 0, hint);
  }
}

void addRGBParam(OfxParamSetHandle paramSet, const char *name, const char *label, Vec3 defaultValue,
                 const char *hint) {
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

} // namespace rimell
