#pragma once

#include "Types.h"

#include "ofxCore.h"
#include "ofxGPURender.h"

namespace rimell {

#ifdef __APPLE__
OfxStatus renderMetalFloat(void *commandQueue, const Image &source, const Image &output,
                           const OfxRectI &renderWindow, const RenderParams &params);
OfxStatus renderMetalCopyFloat(void *commandQueue, const Image &source, const Image &output,
                               const OfxRectI &renderWindow);
inline OfxStatus renderMetalCopy(void *commandQueue, const Image &source, const Image &output,
                                 const OfxRectI &renderWindow) {
  return renderMetalCopyFloat(commandQueue, source, output, renderWindow);
}
void clearMetalPipelineCaches();
#else
inline OfxStatus renderMetalFloat(void *, const Image &, const Image &, const OfxRectI &,
                                  const RenderParams &) {
  return kOfxStatGPURenderFailed;
}

inline OfxStatus renderMetalCopyFloat(void *, const Image &, const Image &, const OfxRectI &) {
  return kOfxStatGPURenderFailed;
}

inline OfxStatus renderMetalCopy(void *, const Image &, const Image &, const OfxRectI &) {
  return kOfxStatGPURenderFailed;
}

inline void clearMetalPipelineCaches() {}
#endif

} // namespace rimell
