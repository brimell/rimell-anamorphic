#pragma once

#include "Types.h"

#include "ofxParam.h"

#include <initializer_list>

namespace rimell {

void addDoubleParam(OfxParamSetHandle paramSet, const char *name, const char *label, double defaultValue,
                    double minValue, double maxValue, double displayMin, double displayMax,
                    const char *hint = nullptr);
void addIntParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                 int minValue, int maxValue, const char *hint = nullptr);
void addBooleanParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                     const char *hint = nullptr);
void addStringParam(OfxParamSetHandle paramSet, const char *name, const char *label,
                    const char *defaultValue = "", const char *hint = nullptr);
void addGroupParam(OfxParamSetHandle paramSet, const char *name, const char *label, int open = 1);
void addPageParam(OfxParamSetHandle paramSet, const char *name, const char *label);
void addChoiceParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                    std::initializer_list<const char *> options, const char *hint = nullptr);
void addRGBParam(OfxParamSetHandle paramSet, const char *name, const char *label, Vec3 defaultValue,
                 const char *hint = nullptr);
void setParamFlags(OfxParamSetHandle paramSet, const char *name, int secret, int enabled, int persistent,
                   int evaluateOnChange);
void setParamParent(OfxParamSetHandle paramSet, const char *name, const char *parent);
void addPageChild(OfxParamSetHandle paramSet, const char *pageName, const char *childName);

} // namespace rimell
