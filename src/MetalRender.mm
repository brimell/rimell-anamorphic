#include "MetalRender.h"

#include "Constants.h"
#include "Diagnostics.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <dlfcn.h>
#include <mutex>

#ifndef RIMELL_METAL_SYNC_DEBUG
#define RIMELL_METAL_SYNC_DEBUG 1
#endif

namespace rimell {
namespace {

struct MetalPipelineCache {
  id<MTLDevice> device = nil;
  id<MTLLibrary> library = nil;
  id<MTLComputePipelineState> mainPipeline = nil;
};

std::mutex &pipelineMutex() {
  static std::mutex mutex;
  return mutex;
}

MetalPipelineCache &pipelineCache() {
  static MetalPipelineCache cache;
  return cache;
}

void resetCache(MetalPipelineCache &cache) {
  [cache.mainPipeline release];
  [cache.library release];
  cache.mainPipeline = nil;
  cache.library = nil;
  cache.device = nil;
}

NSString *metallibPath() {
  NSBundle *bundle = [NSBundle bundleWithIdentifier:@(kPluginIdentifier)];
  NSString *path = [bundle pathForResource:@"RimellKernels" ofType:@"metallib"];
  if (path.length > 0) {
    return path;
  }

  Dl_info info{};
  if (dladdr(reinterpret_cast<const void *>(&renderMetal), &info) == 0 || !info.dli_fname) {
    return nil;
  }

  NSString *binaryPath = [NSString stringWithUTF8String:info.dli_fname];
  NSString *contentsPath =
      [[binaryPath stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
  return [[contentsPath stringByAppendingPathComponent:@"Resources"]
      stringByAppendingPathComponent:@"RimellKernels.metallib"];
}

bool ensurePipelines(MetalPipelineCache &cache, id<MTLDevice> device) {
  std::lock_guard<std::mutex> lock(pipelineMutex());
  if (cache.device == device && cache.mainPipeline) {
    return true;
  }

  resetCache(cache);
  cache.device = device;

  NSString *path = metallibPath();
  if (path.length == 0) {
    logMessage(LogLevel::Error, "render.gpu", "RimellKernels.metallib not found in bundle resources");
    resetCache(cache);
    return false;
  }

  NSError *error = nil;
  NSURL *url = [NSURL fileURLWithPath:path];
  cache.library = [device newLibraryWithURL:url error:&error];
  if (!cache.library) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to load metallib path=%s error=%s",
              [path UTF8String],
              error ? [[error localizedDescription] UTF8String] : "(none)");
    resetCache(cache);
    return false;
  }

  id<MTLFunction> function = [cache.library newFunctionWithName:@"rimell_anamorphic_main"];
  if (!function) {
    logMessage(LogLevel::Error, "render.gpu", "rimell_anamorphic_main function missing");
    resetCache(cache);
    return false;
  }

  cache.mainPipeline = [device newComputePipelineStateWithFunction:function error:&error];
  [function release];
  if (!cache.mainPipeline) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to create compute pipeline error=%s",
              error ? [[error localizedDescription] UTF8String] : "(none)");
    resetCache(cache);
    return false;
  }

  return true;
}

int rowFloats(const ImageBounds &bounds) {
  return bounds.rowBytes > 0 ? bounds.rowBytes / static_cast<int>(sizeof(float)) : 0;
}

MetalUniforms makeMetalUniforms(const RenderContext &ctx, const RenderParams &params) {
  const ImageBounds &src = ctx.sourceMetal.info;
  const ImageBounds &dst = ctx.outputMetal.info;
  const ImageBounds &dep = ctx.depthMetal.info;
  MetalUniforms u{};

  u.width = src.width;
  u.height = src.height;
  u.renderX1 = ctx.renderWindow.x1;
  u.renderY1 = ctx.renderWindow.y1;
  u.renderX2 = ctx.renderWindow.x2;
  u.renderY2 = ctx.renderWindow.y2;

  u.sourceX1 = src.bounds.x1;
  u.sourceY1 = src.bounds.y1;
  u.sourceX2 = src.bounds.x2;
  u.sourceY2 = src.bounds.y2;
  u.outputX1 = dst.bounds.x1;
  u.outputY1 = dst.bounds.y1;
  u.depthX1 = dep.bounds.x1;
  u.depthY1 = dep.bounds.y1;
  u.depthX2 = dep.bounds.x2;
  u.depthY2 = dep.bounds.y2;

  u.sourceRowFloats = rowFloats(src);
  u.outputRowFloats = rowFloats(dst);
  u.depthRowFloats = rowFloats(dep);

  u.depthAvailable = ctx.depthAvailable ? 1 : 0;
  u.invertDepth = params.depthMapInvert != 0 ? 1 : 0;
  u.debugView = params.previewDepthMap != 0 ? 3 : params.debugView;
  u.renderQuality = params.renderQuality;

  u.mix = params.mix;
  u.focusDepth = params.focusDepth;
  u.focusRange = params.depthFocusRange;
  u.depthFalloff = params.depthFalloff;
  u.depthInfluence = params.depthInfluence;
  u.depthDefocusPixels = params.depthDefocusPixels;
  u.depthBloomBoost = params.depthBloomBoost;

  u.squeezeRatio = params.squeezeRatio;
  u.anamorphicTransfer = params.anamorphicTransfer;
  u.axisWarp = params.axisWarp;
  u.centreProtection = params.centerProtection;
  u.edgeCompressionStart = params.edgeCompressionStart;
  u.edgeCompression = params.edgeCompression;
  u.barrel = params.barrel;
  u.mustache = params.mustache;
  u.verticalCompensation = params.verticalCompensation;

  u.edgeBlur = params.edgeBlur;
  u.horizontalSmear = params.horizontalSmear;
  u.tangentialSmear = params.tangentialSmear;
  u.radialFalloff = params.radialFalloff;

  u.lateralCA = params.lateralCA * params.lateralCAPixelScale;
  u.longitudinalCA = params.longitudinalCA;

  u.flareIntensity = params.flareIntensity;
  u.flareLength = params.flareLength;
  u.flareThreshold = params.flareThreshold;
  u.flareAngle = params.flareAngle;
  u.flareColourR = params.flareColour.r;
  u.flareColourG = params.flareColour.g;
  u.flareColourB = params.flareColour.b;

  u.bloomRadius = params.bloomRadius * params.bloomPixelScale;
  u.bokehStretch = params.bokehStretch * params.bokehStretchScale;
  u.bokehRotation = params.bokehRotation;
  u.veil = params.veil;
  u.highlightCream = params.highlightCream;

  u.ovalVignette = params.ovalVignette;
  u.catEyeStrength = params.catEyeStrength;
  return u;
}

} // namespace

OfxStatus renderMetal(const RenderContext &ctx, const RenderParams &params) {
  ScopedLogTimer timer(LogLevel::Info, "render.gpu", "renderMetal");
  timer.setResult("in_progress");

  @autoreleasepool {
    id<MTLCommandQueue> queue = ctx.metalQueue;
    if (!queue || !ctx.sourceMetal.buffer || !ctx.outputMetal.buffer) {
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLDevice> device = queue.device;
    if (!device) {
      logMessage(LogLevel::Error, "render.gpu", "host Metal command queue has no device");
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    MetalPipelineCache &cache = pipelineCache();
    if (!ensurePipelines(cache, device)) {
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    MetalUniforms uniforms = makeMetalUniforms(ctx, params);
    static_assert(sizeof(MetalUniforms) % 4 == 0, "MetalUniforms must stay scalar-aligned");
    id<MTLBuffer> uniformBuffer = [device newBufferWithBytes:&uniforms
                                                      length:sizeof(MetalUniforms)
                                                     options:MTLResourceStorageModeShared];
    if (!uniformBuffer) {
      timer.setResult(ofxStatusToString(kOfxStatErrMemory));
      return kOfxStatErrMemory;
    }

    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    if (!commandBuffer) {
      [uniformBuffer release];
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    if (!encoder) {
      [uniformBuffer release];
      timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
      return kOfxStatGPURenderFailed;
    }

    [encoder setComputePipelineState:cache.mainPipeline];
    [encoder setBuffer:ctx.sourceMetal.buffer offset:0 atIndex:0];
    [encoder setBuffer:ctx.outputMetal.buffer offset:0 atIndex:1];
    [encoder setBuffer:(ctx.depthAvailable && ctx.depthMetal.buffer ? ctx.depthMetal.buffer
                                                                    : ctx.sourceMetal.buffer)
                offset:0
               atIndex:2];
    [encoder setBuffer:uniformBuffer offset:0 atIndex:3];

    const int renderWidth = std::max(0, ctx.renderWindow.x2 - ctx.renderWindow.x1);
    const int renderHeight = std::max(0, ctx.renderWindow.y2 - ctx.renderWindow.y1);
    const NSUInteger threadWidth = std::max<NSUInteger>(1, cache.mainPipeline.threadExecutionWidth);
    const NSUInteger threadHeight =
        std::max<NSUInteger>(1, cache.mainPipeline.maxTotalThreadsPerThreadgroup / threadWidth);
    MTLSize threadsPerGroup = MTLSizeMake(threadWidth, threadHeight, 1);
    MTLSize groups = MTLSizeMake((renderWidth + static_cast<int>(threadWidth) - 1) /
                                     static_cast<int>(threadWidth),
                                 (renderHeight + static_cast<int>(threadHeight) - 1) /
                                     static_cast<int>(threadHeight),
                                 1);

    [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threadsPerGroup];
    [encoder endEncoding];
    [commandBuffer commit];

#if RIMELL_METAL_SYNC_DEBUG
    [commandBuffer waitUntilCompleted];
#endif

    const bool failed = commandBuffer.status == MTLCommandBufferStatusError || commandBuffer.error;
    if (failed) {
      logPrintf(LogLevel::Error,
                "render.gpu",
                "command buffer failed error=%s",
                commandBuffer.error ? [[commandBuffer.error localizedDescription] UTF8String] : "(none)");
    }

    [uniformBuffer release];
    const OfxStatus status = failed ? kOfxStatGPURenderFailed : kOfxStatOK;
    timer.setResult(ofxStatusToString(status));
    return status;
  }
}

} // namespace rimell
