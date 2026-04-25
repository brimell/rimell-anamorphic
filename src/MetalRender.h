#pragma once

#include "RenderTypes.h"

#include "ofxCore.h"
#include "ofxGPURender.h"

namespace rimell {

#ifdef __APPLE__
OfxStatus renderMetal(const RenderContext &ctx, const RenderParams &params);
#else
inline OfxStatus renderMetal(const RenderContext &, const RenderParams &) {
  return kOfxStatGPURenderFailed;
}
#endif

} // namespace rimell
