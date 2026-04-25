#pragma once

#include "ofxImageEffect.h"

namespace rimell {

OfxStatus describe(OfxImageEffectHandle effect);
OfxStatus describeInContext(OfxImageEffectHandle effect);

} // namespace rimell
