#include "MetalRender.h"

#include "Diagnostics.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <cmath>
#include <dlfcn.h>
#include <sys/stat.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <type_traits>

namespace rimell {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr int kUniformVersion = 1;

struct MetalParams {
  float mix;
  int uniformVersion;
  int debugView;
  int renderQuality;
  int inputMode;
  int squeezeMode;
  float anamorphicTransfer;
  int lensIdentity;
  float squeezeRatio;
  float axisWarp;
  float centerProtection;
  float edgeCompressionStart;
  float horizontalFovBoost;
  float virtualFocalLength;
  float breathingScale;
  float bokehStretch;
  float bokehRotation;
  float bokehEdgeFalloff;
  float bokehStretchScale;
  float bloomPixelScale;
  float bloomThresholdScale;
  int bloomRings;
  int bloomSamplesPerRing;
  float bloomEdgeKeepScale;
  float bloomVeilScale;
  float bloomCreamScale;
  float flareIntensity;
  float flareLength;
  float flareColourR;
  float flareColourG;
  float flareColourB;
  float flareThreshold;
  float flareAngle;
  float flareStepDensity;
  float flareSpanScale;
  float flareFalloff;
  float veil;
  float bloomRadius;
  float highlightCream;
  float blackLiftProtection;
  int ghostCount;
  float ghostSpread;
  float ghostTintR;
  float ghostTintG;
  float ghostTintB;
  float ghostIntensity;
  int coatingStyle;
  float coatingWarmResponse;
  float coatingCoolResponse;
  float edgeBlur;
  float tangentialSmear;
  float radialFalloff;
  float edgeBlurPixels;
  float fieldCurvaturePixels;
  float smearPixels;
  float barrel;
  float mustache;
  float verticalCompensation;
  float verticalCompensationScale;
  float closeFocusMumps;
  float faceWidthCompensation;
  float focusDistance;
  float breathingAmount;
  float mumpsScale;
  float lateralCA;
  float longitudinalCA;
  float edgeOnlyCA;
  float lateralCAPixelScale;
  float ovalVignette;
  float vignetteAsymmetry;
  float cornerBias;
  float ovalVignetteScale;
  float vignetteAsymmetryScale;
  float horizontalSmear;
  float verticalSharpness;
  float fieldCurvature;
  float catEyeStrength;
  float bokehVignette;
  float edgeCompression;
  float catEyeDimScale;
  float bokehVignetteDimScale;
  float edgeCompressionScale;
  float centerVeilScale;
  int guidesEnabled;
  int outputAspect;
  float customOutputAspect;
  float safeArea;
  int letterboxPreview;
  float letterboxOpacity;
  float guideAspectStrength;
  float guideSafeStrength;
  int autoEdgeCrop;
  float edgeCropScale;
  float flareDirX;
  float flareDirY;
  float bokehCos;
  float bokehSin;
  float lensIdentityBloomScale;
  float lensIdentityFlareScale;
  float lensIdentityGhostScaleX;
  float lensIdentityGhostScaleY;
  float safeSqueezeDelta;
};

struct MetalImageInfo {
  int sourceX1;
  int sourceY1;
  int sourceX2;
  int sourceY2;
  int outputX1;
  int outputY1;
  int sourceRowFloats;
  int outputRowFloats;
  int width;
  int height;
  int renderX1;
  int renderY1;
  int renderX2;
  int renderY2;
};

struct CopyUniforms {
  int sourceX1;
  int sourceY1;
  int sourceX2;
  int sourceY2;
  int sourceRowFloats;
  int outputX1;
  int outputY1;
  int outputRowFloats;
  int renderX1;
  int renderY1;
  int renderX2;
  int renderY2;
};

static_assert(std::is_standard_layout_v<MetalParams>);
static_assert(std::is_trivially_copyable_v<MetalParams>);
static_assert(std::is_standard_layout_v<MetalImageInfo>);
static_assert(std::is_trivially_copyable_v<MetalImageInfo>);
static_assert(std::is_standard_layout_v<CopyUniforms>);
static_assert(std::is_trivially_copyable_v<CopyUniforms>);

using DeviceKey = const void *;

MetalParams packParams(const RenderParams &params) {
  const float safeSqueezeDelta = std::max(0.0f, params.squeezeRatio - 1.0f);
  const float flareRadians = params.flareAngle * kPi / 180.0f;
  const float bokehRadians = params.bokehRotation * kPi / 180.0f;
  const float flareScale = (1.0f + safeSqueezeDelta * 0.55f);
  const float bloomScale = (1.0f + safeSqueezeDelta * 0.35f);
  const float ghostScaleX = (1.0f + safeSqueezeDelta * 0.12f);
  const float ghostScaleY = 1.0f / (1.0f + safeSqueezeDelta * 0.06f);

  return {
      params.mix,
      kUniformVersion,
      params.debugView,
      params.renderQuality,
      params.inputMode,
      params.squeezeMode,
      params.anamorphicTransfer,
      params.lensIdentity,
      params.squeezeRatio,
      params.axisWarp,
      params.centerProtection,
      params.edgeCompressionStart,
      params.horizontalFovBoost,
      params.virtualFocalLength,
      params.breathingScale,
      params.bokehStretch,
      params.bokehRotation,
      params.bokehEdgeFalloff,
      params.bokehStretchScale,
      params.bloomPixelScale,
      params.bloomThresholdScale,
      params.bloomRings,
      params.bloomSamplesPerRing,
      params.bloomEdgeKeepScale,
      params.bloomVeilScale,
      params.bloomCreamScale,
      params.flareIntensity,
      params.flareLength,
      params.flareColour.r,
      params.flareColour.g,
      params.flareColour.b,
      params.flareThreshold,
      params.flareAngle,
      params.flareStepDensity,
      params.flareSpanScale,
      params.flareFalloff,
      params.veil,
      params.bloomRadius,
      params.highlightCream,
      params.blackLiftProtection,
      params.ghostCount,
      params.ghostSpread,
      params.ghostTint.r,
      params.ghostTint.g,
      params.ghostTint.b,
      params.ghostIntensity,
      params.coatingStyle,
      params.coatingWarmResponse,
      params.coatingCoolResponse,
      params.edgeBlur,
      params.tangentialSmear,
      params.radialFalloff,
      params.edgeBlurPixels,
      params.fieldCurvaturePixels,
      params.smearPixels,
      params.barrel,
      params.mustache,
      params.verticalCompensation,
      params.verticalCompensationScale,
      params.closeFocusMumps,
      params.faceWidthCompensation,
      params.focusDistance,
      params.breathingAmount,
      params.mumpsScale,
      params.lateralCA,
      params.longitudinalCA,
      params.edgeOnlyCA,
      params.lateralCAPixelScale,
      params.ovalVignette,
      params.vignetteAsymmetry,
      params.cornerBias,
      params.ovalVignetteScale,
      params.vignetteAsymmetryScale,
      params.horizontalSmear,
      params.verticalSharpness,
      params.fieldCurvature,
      params.catEyeStrength,
      params.bokehVignette,
      params.edgeCompression,
      params.catEyeDimScale,
      params.bokehVignetteDimScale,
      params.edgeCompressionScale,
      params.centerVeilScale,
      params.guidesEnabled,
      params.outputAspect,
      params.customOutputAspect,
      params.safeArea,
      params.letterboxPreview,
      params.letterboxOpacity,
      params.guideAspectStrength,
      params.guideSafeStrength,
      params.autoEdgeCrop,
      params.edgeCropScale,
      std::cos(flareRadians),
      std::sin(flareRadians),
      std::cos(bokehRadians),
      std::sin(bokehRadians),
      bloomScale,
      flareScale,
      ghostScaleX,
      ghostScaleY,
      safeSqueezeDelta,
  };
}

bool validateMetalImageLayout(const Image &image) {
  const int width = image.bounds.x2 - image.bounds.x1;
  const int height = image.bounds.y2 - image.bounds.y1;
  return image.storage == ImageStorage::Metal && image.data && image.rowBytes > 0 && width > 0 &&
         height > 0 && image.rowBytes % static_cast<int>(sizeof(float)) == 0;
}

bool renderWindowWithinOutput(const OfxRectI &renderWindow, const Image &output) {
  return renderWindow.x1 >= output.bounds.x1 && renderWindow.y1 >= output.bounds.y1 &&
         renderWindow.x2 <= output.bounds.x2 && renderWindow.y2 <= output.bounds.y2;
}

bool fileExists(const std::string &path) {
  struct stat st {};
  return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string metallibPath() {
  static std::once_flag once;
  static std::string path;
  std::call_once(once, [] {
    Dl_info info{};
    if (dladdr(reinterpret_cast<const void *>(&renderMetalFloat), &info) == 0 || !info.dli_fname) {
      logMessage(LogLevel::Error, "render.gpu", "failed to resolve plugin image for metallib lookup");
      return;
    }

    std::string binaryPath(info.dli_fname);
    const std::string::size_type binarySlash = binaryPath.find_last_of('/');
    if (binarySlash == std::string::npos) {
      return;
    }

    const std::string contentsCandidate = binaryPath.substr(0, binarySlash);
    const std::string::size_type contentsSlash = contentsCandidate.find_last_of('/');
    if (contentsSlash == std::string::npos) {
      return;
    }

    const std::string contentsDir = contentsCandidate.substr(0, contentsSlash);
    path = contentsDir + "/Resources/RimellKernels.metallib";
    logPrintf(LogLevel::Debug, "render.gpu", "resolved metallib path=%s", path.c_str());
  });
  return path;
}

id<MTLDevice> deviceFromQueue(id<MTLCommandQueue> queue) {
  return queue ? queue.device : nil;
}

id<MTLLibrary> loadLibrary(id<MTLDevice> device) {
  NSError *error = nil;
  const std::string path = metallibPath();
  if (path.empty()) {
    logMessage(LogLevel::Error, "render.gpu", "metallib path is empty");
    return nil;
  }
  if (!fileExists(path)) {
    logPrintf(LogLevel::Error, "render.gpu", "metallib does not exist at path=%s", path.c_str());
    return nil;
  }

  NSString *libraryPath = [[NSString alloc] initWithUTF8String:path.c_str()];
  id<MTLLibrary> library = [device newLibraryWithFile:libraryPath error:&error];
  [libraryPath release];
  if (!library) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to load metallib path=%s error=%s",
              path.c_str(),
              error ? [[error localizedDescription] UTF8String] : "(none)");
  }
  return library;
}

template <typename MapT>
void releasePipelineCache(MapT &cache) {
  for (auto &entry : cache) {
    [entry.second release];
    entry.second = nil;
  }
  cache.clear();
}

std::mutex pipelineMutex;
std::unordered_map<DeviceKey, id<MTLComputePipelineState>> floatPipelineCache;
std::unordered_map<DeviceKey, id<MTLComputePipelineState>> copyPipelineCache;

DeviceKey deviceKey(id<MTLDevice> device) {
  return (__bridge const void *)device;
}

id<MTLComputePipelineState> pipelineForDevice(id<MTLDevice> device) {
  if (!device) {
    return nil;
  }

  std::lock_guard<std::mutex> lock(pipelineMutex);
  const DeviceKey key = deviceKey(device);
  const auto it = floatPipelineCache.find(key);
  if (it != floatPipelineCache.end()) {
    return it->second;
  }

  id<MTLLibrary> library = loadLibrary(device);
  if (!library) {
    return nil;
  }

  NSError *error = nil;
  id<MTLFunction> function = [library newFunctionWithName:@"RimellAnamorphicFloat"];
  if (!function) {
    logMessage(LogLevel::Error, "render.gpu", "failed to find RimellAnamorphicFloat in metallib");
    [library release];
    return nil;
  }

  id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
  [function release];
  [library release];
  if (!pipeline) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to build float pipeline error=%s",
              error ? [[error localizedDescription] UTF8String] : "(none)");
    return nil;
  }

  floatPipelineCache[key] = pipeline;
  return pipeline;
}

id<MTLComputePipelineState> copyPipelineForDevice(id<MTLDevice> device) {
  if (!device) {
    return nil;
  }

  std::lock_guard<std::mutex> lock(pipelineMutex);
  const DeviceKey key = deviceKey(device);
  const auto it = copyPipelineCache.find(key);
  if (it != copyPipelineCache.end()) {
    return it->second;
  }

  id<MTLLibrary> library = loadLibrary(device);
  if (!library) {
    return nil;
  }

  NSError *error = nil;
  id<MTLFunction> function = [library newFunctionWithName:@"rimell_copy_float"];
  if (!function) {
    logMessage(LogLevel::Error, "render.gpu", "failed to find rimell_copy_float in metallib");
    [library release];
    return nil;
  }

  id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
  [function release];
  [library release];
  if (!pipeline) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to build copy pipeline error=%s",
              error ? [[error localizedDescription] UTF8String] : "(none)");
    return nil;
  }

  copyPipelineCache[key] = pipeline;
  return pipeline;
}

OfxStatus renderMetalTyped(void *commandQueue, const Image &source, const Image &output,
                           const OfxRectI &renderWindow, const RenderParams &params) {
  @autoreleasepool {
    ScopedLogTimer timer(LogLevel::Info, "render.gpu", "renderMetalTyped");
    timer.setResult("in_progress");

    if (!commandQueue || !source.data || !output.data) {
      logMessage(LogLevel::Error, "render.gpu", "invalid queue/source/output pointers");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    if (source.bounds.x1 != output.bounds.x1 || source.bounds.y1 != output.bounds.y1 ||
        source.bounds.x2 != output.bounds.x2 || source.bounds.y2 != output.bounds.y2) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "source/output bounds differ source=[%d,%d,%d,%d] output=[%d,%d,%d,%d]",
                source.bounds.x1,
                source.bounds.y1,
                source.bounds.x2,
                source.bounds.y2,
                output.bounds.x1,
                output.bounds.y1,
                output.bounds.x2,
                output.bounds.y2);
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)commandQueue;
    id<MTLDevice> device = deviceFromQueue(queue);
    if (!queue || !device) {
      logMessage(LogLevel::Error, "render.gpu", "invalid Metal command queue");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLBuffer> sourceBuffer = (__bridge id<MTLBuffer>)source.data;
    id<MTLBuffer> outputBuffer = (__bridge id<MTLBuffer>)output.data;
    if (!sourceBuffer || !outputBuffer || sourceBuffer.device != device || outputBuffer.device != device) {
      logMessage(LogLevel::Error, "render.gpu", "invalid Metal buffers or device mismatch");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    if (!validateMetalImageLayout(source) || !validateMetalImageLayout(output) ||
        !renderWindowWithinOutput(renderWindow, output)) {
      logMessage(LogLevel::Error, "render.gpu", "invalid Metal source/output layout or render window");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLComputePipelineState> pipeline = pipelineForDevice(device);
    if (!pipeline) {
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    const int renderWidth = renderWindow.x2 - renderWindow.x1;
    const int renderHeight = renderWindow.y2 - renderWindow.y1;
    if (renderWidth <= 0 || renderHeight <= 0) {
      logPrintf(LogLevel::Error, "render.gpu", "invalid render window width=%d height=%d", renderWidth, renderHeight);
      timer.setResult(ofxStatusToString(kOfxStatErrValue));
      return kOfxStatErrValue;
    }

    const int channelBytes = static_cast<int>(sizeof(float));
    const int sourceRowFloats = source.rowBytes / channelBytes;
    const int outputRowFloats = output.rowBytes / channelBytes;
    if (sourceRowFloats <= 0 || outputRowFloats <= 0 || source.rowBytes % channelBytes != 0 ||
        output.rowBytes % channelBytes != 0) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "invalid rowBytes source=%d output=%d channelBytes=%d",
                source.rowBytes,
                output.rowBytes,
                channelBytes);
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    const NSUInteger requiredSourceBytes =
        static_cast<NSUInteger>(source.rowBytes) * static_cast<NSUInteger>(source.bounds.y2 - source.bounds.y1);
    const NSUInteger requiredOutputBytes =
        static_cast<NSUInteger>(output.rowBytes) * static_cast<NSUInteger>(output.bounds.y2 - output.bounds.y1);
    if (sourceBuffer.length < requiredSourceBytes || outputBuffer.length < requiredOutputBytes) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "Metal buffer too small sourceLen=%llu requiredSource=%llu outputLen=%llu requiredOutput=%llu",
                static_cast<unsigned long long>(sourceBuffer.length),
                static_cast<unsigned long long>(requiredSourceBytes),
                static_cast<unsigned long long>(outputBuffer.length),
                static_cast<unsigned long long>(requiredOutputBytes));
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    MetalParams packedParams = packParams(params);
    MetalImageInfo info{
        source.bounds.x1,
        source.bounds.y1,
        source.bounds.x2,
        source.bounds.y2,
        output.bounds.x1,
        output.bounds.y1,
        sourceRowFloats,
        outputRowFloats,
        source.bounds.x2 - source.bounds.x1,
        source.bounds.y2 - source.bounds.y1,
        renderWindow.x1,
        renderWindow.y1,
        renderWindow.x2,
        renderWindow.y2,
    };

    id<MTLBuffer> paramsBuffer = [device newBufferWithBytes:&packedParams
                                                     length:sizeof(packedParams)
                                                    options:MTLResourceStorageModeShared];
    id<MTLBuffer> infoBuffer = [device newBufferWithBytes:&info
                                                   length:sizeof(info)
                                                  options:MTLResourceStorageModeShared];
    if (!paramsBuffer || !infoBuffer) {
      logMessage(LogLevel::Error, "render.gpu", "failed to create Metal uniform buffers");
      [paramsBuffer release];
      [infoBuffer release];
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    if (!commandBuffer) {
      logMessage(LogLevel::Error, "render.gpu", "failed to create Metal command buffer");
      [paramsBuffer release];
      [infoBuffer release];
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    logPrintf(LogLevel::Trace,
              "render.gpu",
              "commandBuffer retainedReferences=%d",
              commandBuffer.retainedReferences ? 1 : 0);

    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    if (!encoder) {
      logMessage(LogLevel::Error, "render.gpu", "failed to create Metal command encoder");
      [paramsBuffer release];
      [infoBuffer release];
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    [encoder setComputePipelineState:pipeline];
    [encoder setBuffer:sourceBuffer offset:0 atIndex:0];
    [encoder setBuffer:outputBuffer offset:0 atIndex:1];
    [encoder setBuffer:paramsBuffer offset:0 atIndex:2];
    [encoder setBuffer:infoBuffer offset:0 atIndex:3];

    const NSUInteger threadWidth = std::max<NSUInteger>(1, pipeline.threadExecutionWidth);
    const NSUInteger maxThreads = std::max<NSUInteger>(threadWidth, pipeline.maxTotalThreadsPerThreadgroup);
    const NSUInteger threadHeight = std::min<NSUInteger>(std::max<NSUInteger>(1, maxThreads / threadWidth), 16);
    const MTLSize threadsPerGroup = MTLSizeMake(threadWidth, threadHeight, 1);
    const MTLSize gridSize = MTLSizeMake(static_cast<NSUInteger>(renderWidth), static_cast<NSUInteger>(renderHeight), 1);

    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadsPerGroup];
    [encoder endEncoding];
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cb) {
      if (cb.error) {
        logPrintf(LogLevel::Error,
                  "render.gpu",
                  "command buffer failed: %s",
                  [[cb.error localizedDescription] UTF8String]);
      }
      [paramsBuffer release];
      [infoBuffer release];
    }];
    [commandBuffer commit];
    timer.setResult(ofxStatusToString(kOfxStatOK));
    return kOfxStatOK;
  }
}

} // namespace

} // namespace rimell

namespace rimell {

OfxStatus renderMetalFloat(void *commandQueue, const Image &source, const Image &output,
                           const OfxRectI &renderWindow, const RenderParams &params) {
  return renderMetalTyped(commandQueue, source, output, renderWindow, params);
}

OfxStatus renderMetalCopyFloat(void *commandQueue, const Image &source, const Image &output,
                               const OfxRectI &renderWindow) {
  @autoreleasepool {
    ScopedLogTimer timer(LogLevel::Info, "render.gpu", "renderMetalCopyFloat");
    timer.setResult("in_progress");

    if (!commandQueue || !source.data || !output.data) {
      logMessage(LogLevel::Error, "render.gpu", "invalid queue/source/output pointers");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    if (source.bounds.x1 != output.bounds.x1 || source.bounds.y1 != output.bounds.y1 ||
        source.bounds.x2 != output.bounds.x2 || source.bounds.y2 != output.bounds.y2) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "source/output bounds differ source=[%d,%d,%d,%d] output=[%d,%d,%d,%d]",
                source.bounds.x1,
                source.bounds.y1,
                source.bounds.x2,
                source.bounds.y2,
                output.bounds.x1,
                output.bounds.y1,
                output.bounds.x2,
                output.bounds.y2);
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)commandQueue;
    id<MTLDevice> device = deviceFromQueue(queue);
    if (!queue || !device) {
      logMessage(LogLevel::Error, "render.gpu", "invalid Metal command queue");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLBuffer> sourceBuffer = (__bridge id<MTLBuffer>)source.data;
    id<MTLBuffer> outputBuffer = (__bridge id<MTLBuffer>)output.data;
    if (!sourceBuffer || !outputBuffer || sourceBuffer.device != device || outputBuffer.device != device) {
      logMessage(LogLevel::Error, "render.gpu", "invalid Metal buffers or device mismatch");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    const int renderWidth = renderWindow.x2 - renderWindow.x1;
    const int renderHeight = renderWindow.y2 - renderWindow.y1;
    if (renderWidth <= 0 || renderHeight <= 0) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "invalid render window width=%d height=%d",
                renderWidth,
                renderHeight);
      timer.setResult(ofxStatusToString(kOfxStatErrValue));
      return kOfxStatErrValue;
    }

    if (!validateMetalImageLayout(source) || !validateMetalImageLayout(output) ||
        !renderWindowWithinOutput(renderWindow, output)) {
      logMessage(LogLevel::Error, "render.gpu", "invalid Metal source/output layout or render window");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    const int channelBytes = static_cast<int>(sizeof(float));
    const int sourceRowFloats = source.rowBytes / channelBytes;
    const int outputRowFloats = output.rowBytes / channelBytes;
    if (sourceRowFloats <= 0 || outputRowFloats <= 0 || source.rowBytes % channelBytes != 0 ||
        output.rowBytes % channelBytes != 0) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "invalid rowBytes source=%d output=%d channelBytes=%d",
                source.rowBytes,
                output.rowBytes,
                channelBytes);
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    const NSUInteger requiredSourceBytes =
        static_cast<NSUInteger>(source.rowBytes) * static_cast<NSUInteger>(source.bounds.y2 - source.bounds.y1);
    const NSUInteger requiredOutputBytes =
        static_cast<NSUInteger>(output.rowBytes) * static_cast<NSUInteger>(output.bounds.y2 - output.bounds.y1);
    if (sourceBuffer.length < requiredSourceBytes || outputBuffer.length < requiredOutputBytes) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "Metal buffer too small sourceLen=%llu requiredSource=%llu outputLen=%llu requiredOutput=%llu",
                static_cast<unsigned long long>(sourceBuffer.length),
                static_cast<unsigned long long>(requiredSourceBytes),
                static_cast<unsigned long long>(outputBuffer.length),
                static_cast<unsigned long long>(requiredOutputBytes));
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLComputePipelineState> pipeline = copyPipelineForDevice(device);
    if (!pipeline) {
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    CopyUniforms uniforms{
        source.bounds.x1,
        source.bounds.y1,
        source.bounds.x2,
        source.bounds.y2,
        sourceRowFloats,
        output.bounds.x1,
        output.bounds.y1,
        outputRowFloats,
        renderWindow.x1,
        renderWindow.y1,
        renderWindow.x2,
        renderWindow.y2,
    };

    id<MTLBuffer> uniformsBuffer = [device newBufferWithBytes:&uniforms
                                                       length:sizeof(uniforms)
                                                      options:MTLResourceStorageModeShared];
    if (!uniformsBuffer) {
      logMessage(LogLevel::Error, "render.gpu", "failed to create Metal copy uniform buffer");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    if (!commandBuffer) {
      logMessage(LogLevel::Error, "render.gpu", "failed to create Metal copy command buffer");
      [uniformsBuffer release];
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    logPrintf(LogLevel::Trace,
              "render.gpu",
              "copy commandBuffer retainedReferences=%d",
              commandBuffer.retainedReferences ? 1 : 0);

    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    if (!encoder) {
      logMessage(LogLevel::Error, "render.gpu", "failed to create Metal copy command encoder");
      [uniformsBuffer release];
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    [encoder setComputePipelineState:pipeline];
    [encoder setBuffer:sourceBuffer offset:0 atIndex:0];
    [encoder setBuffer:outputBuffer offset:0 atIndex:1];
    [encoder setBuffer:uniformsBuffer offset:0 atIndex:2];

    const NSUInteger threadWidth = std::max<NSUInteger>(1, pipeline.threadExecutionWidth);
    const NSUInteger maxThreads = std::max<NSUInteger>(threadWidth, pipeline.maxTotalThreadsPerThreadgroup);
    const NSUInteger threadHeight = std::min<NSUInteger>(std::max<NSUInteger>(1, maxThreads / threadWidth), 16);
    const MTLSize threadsPerGroup = MTLSizeMake(threadWidth, threadHeight, 1);
    const MTLSize gridSize = MTLSizeMake(static_cast<NSUInteger>(renderWidth), static_cast<NSUInteger>(renderHeight), 1);

    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadsPerGroup];
    [encoder endEncoding];
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cb) {
      if (cb.error) {
        logPrintf(LogLevel::Error,
                  "render.gpu",
                  "copy command buffer failed: %s",
                  [[cb.error localizedDescription] UTF8String]);
      }
      [uniformsBuffer release];
    }];
    [commandBuffer commit];
    timer.setResult(ofxStatusToString(kOfxStatOK));
    return kOfxStatOK;
  }
}

void clearMetalPipelineCaches() {
  std::lock_guard<std::mutex> lock(pipelineMutex);
  releasePipelineCache(floatPipelineCache);
  releasePipelineCache(copyPipelineCache);
}

} // namespace rimell
