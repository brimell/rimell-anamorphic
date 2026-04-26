#pragma once

#include "Types.h"

#include "ofxCore.h"
#include "ofxGPURender.h"

namespace rimell {

#ifdef __APPLE__
OfxStatus renderMetalFloat(void *commandQueue, const Image &source, const Image &output,
                           const OfxRectI &renderWindow, const RenderParams &params);
#else
inline OfxStatus renderMetalFloat(void *, const Image &, const Image &, const OfxRectI &,
                                  const RenderParams &) {
  return kOfxStatGPURenderFailed;
}
#endif

} // namespace rimell
