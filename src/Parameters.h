#pragma once

#include "Types.h"

#include "ofxImageEffect.h"
#include <string>

namespace rimell {

RenderParams readParams(OfxImageEffectHandle effect);
RenderParams readParams(OfxImageEffectHandle effect, OfxTime time);
float aspectValue(int index, float customOutputAspect);
const char *processingBackendName(int backend);

std::string getStringParam(OfxParamSetHandle paramSet, const char *name, const std::string &fallback = "");

} // namespace rimell
