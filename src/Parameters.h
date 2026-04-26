#pragma once

#include "Types.h"

#include "ofxImageEffect.h"

namespace rimell {

RenderParams readParams(OfxImageEffectHandle effect);
RenderParams readParams(OfxImageEffectHandle effect, OfxTime time);
float aspectValue(int index, float customOutputAspect);
const char *processingBackendName(int backend);

} // namespace rimell
