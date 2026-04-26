#pragma once

#include "Types.h"

#include "ofxCore.h"
#include "ofxGPURender.h"

namespace rimell {

#ifdef __APPLE__
OfxStatus renderMetalFloat(void *commandQueue, const Image &source, const Image &output,
                           const OfxRectI &renderWindow, const RenderParams &params);
OfxStatus renderMetalCopy(void *commandQueue, const Image &source, const Image &output,
                          const OfxRectI &renderWindow);
#else
inline OfxStatus renderMetalFloat(void *, const Image &, const Image &, const OfxRectI &,
                                  const RenderParams &) {
  return kOfxStatGPURenderFailed;
}

inline OfxStatus renderMetalCopy(void *, const Image &, const Image &, const OfxRectI &) {
  return kOfxStatGPURenderFailed;
}
#endif

} // namespace rimell
