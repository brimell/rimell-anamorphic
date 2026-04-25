#pragma once

#include "ofxCore.h"
#include "ofxImageEffect.h"

namespace rimell {

OfxStatus render(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs);
OfxStatus isIdentity(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs,
                     OfxPropertySetHandle outArgs);
OfxStatus getRegionOfDefinition(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs,
                                OfxPropertySetHandle outArgs);
OfxStatus getRegionsOfInterest(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs,
                               OfxPropertySetHandle outArgs);
OfxStatus getClipPreferences(OfxImageEffectHandle instance, OfxPropertySetHandle outArgs);

} // namespace rimell
