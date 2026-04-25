#pragma once

#include "Types.h"

#include "ofxParam.h"

namespace rimell {

void addDoubleParam(OfxParamSetHandle paramSet, const char *name, const char *label, double defaultValue,
                    double minValue, double maxValue, double displayMin, double displayMax,
                    const char *hint = nullptr);
void addIntParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                 int minValue, int maxValue, const char *hint = nullptr);
void addBooleanParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                     const char *hint = nullptr);
void addGroupParam(OfxParamSetHandle paramSet, const char *name, const char *label, int open = 1);
void addChoiceParam(OfxParamSetHandle paramSet, const char *name, const char *label, int defaultValue,
                    const char *option0, const char *option1, const char *option2 = nullptr,
                    const char *option3 = nullptr, const char *option4 = nullptr,
                    const char *hint = nullptr);
void addRGBParam(OfxParamSetHandle paramSet, const char *name, const char *label, Vec3 defaultValue,
                 const char *hint = nullptr);
void setParamParent(OfxParamSetHandle paramSet, const char *name, const char *parent);

} // namespace rimell
