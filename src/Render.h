#pragma once

#include "ofxCore.h"
#include "ofxImageEffect.h"

namespace rimell {

OfxStatus render(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs);

} // namespace rimell
