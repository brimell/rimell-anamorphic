#pragma once

#include "Types.h"

#include "ofxCore.h"
#include "ofxImageEffect.h"

#include <atomic>
#include <string>

#ifdef __APPLE__
#ifdef __OBJC__
#import <Metal/Metal.h>
using RimellMetalBuffer = id<MTLBuffer>;
using RimellMetalCommandQueue = id<MTLCommandQueue>;
#else
using RimellMetalBuffer = void *;
using RimellMetalCommandQueue = void *;
#endif
#endif

namespace rimell {

enum class Backend {
  CPU,
  Metal,
};

enum class ProcessingBackend {
  Auto = 0,
  CPU = 1,
  Metal = 2,
};

enum class MetalSafety {
  Strict = 0,
  Experimental = 1,
};

struct ImageBounds {
  OfxRectI bounds{};
  int rowBytes = 0;
  int width = 0;
  int height = 0;
};

struct CpuImage {
  void *data = nullptr;
  ImageBounds info{};
};

#ifdef __APPLE__
struct MetalImage {
  RimellMetalBuffer buffer = nullptr;
  ImageBounds info{};
};
#endif

struct ClipFormat {
  std::string bitDepth;
  std::string components;

  bool isFloatRGBA() const {
    return bitDepth == kOfxBitDepthFloat && components == kOfxImageComponentRGBA;
  }
};

struct RenderContext {
  OfxImageEffectHandle instance = nullptr;
  OfxTime time = 0.0;
  OfxRectI renderWindow{};

  bool sourceConnected = false;
  bool depthConnected = false;
  bool depthAvailable = false;

  ClipFormat sourceFormat{};
  ClipFormat outputFormat{};
  ClipFormat depthFormat{};

  CpuImage sourceCpu{};
  CpuImage outputCpu{};
  CpuImage depthCpu{};

#ifdef __APPLE__
  bool hostMetalEnabled = false;
  bool hostHasMetalQueue = false;
  RimellMetalCommandQueue metalQueue = nullptr;

  bool sourceIsMetal = false;
  bool outputIsMetal = false;
  bool depthIsMetal = false;

  MetalImage sourceMetal{};
  MetalImage outputMetal{};
  MetalImage depthMetal{};
#endif

  Backend backend = Backend::CPU;
};

#ifdef __APPLE__
struct MetalUniforms {
  int width;
  int height;
  int renderX1;
  int renderY1;
  int renderX2;
  int renderY2;

  int sourceX1;
  int sourceY1;
  int sourceX2;
  int sourceY2;
  int outputX1;
  int outputY1;
  int depthX1;
  int depthY1;
  int depthX2;
  int depthY2;

  int sourceRowFloats;
  int outputRowFloats;
  int depthRowFloats;

  int depthAvailable;
  int invertDepth;
  int debugView;
  int renderQuality;

  float mix;
  float focusDepth;
  float focusRange;
  float depthFalloff;
  float depthInfluence;
  float depthDefocusPixels;
  float depthBloomBoost;

  float squeezeRatio;
  float anamorphicTransfer;
  float axisWarp;
  float centreProtection;
  float edgeCompressionStart;
  float edgeCompression;
  float barrel;
  float mustache;
  float verticalCompensation;

  float edgeBlur;
  float horizontalSmear;
  float tangentialSmear;
  float radialFalloff;

  float lateralCA;
  float longitudinalCA;

  float flareIntensity;
  float flareLength;
  float flareThreshold;
  float flareAngle;
  float flareColourR;
  float flareColourG;
  float flareColourB;

  float bloomRadius;
  float bokehStretch;
  float bokehRotation;
  float veil;
  float highlightCream;

  float ovalVignette;
  float catEyeStrength;
};
#endif

struct InstanceData {
  std::atomic<bool> metalDisabledAfterFailure{false};
};

const char *backendName(Backend backend);

} // namespace rimell
