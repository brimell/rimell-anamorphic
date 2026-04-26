#include "MetalRenderer.h"

#if defined(__APPLE__)

#import <Metal/Metal.h>

#include <mutex>
#include <unordered_map>

namespace {

struct SolidUniforms {
  int renderX1;
  int renderY1;
  int renderX2;
  int renderY2;
  int outputX1;
  int outputY1;
  int outputRowFloats;
};

const char *kSolidKernelSource = R"metal(
#include <metal_stdlib>
using namespace metal;

struct SolidUniforms {
    int renderX1;
    int renderY1;
    int renderX2;
    int renderY2;
    int outputX1;
    int outputY1;
    int outputRowFloats;
};

kernel void rimell_solid_colour(
    device float *output [[buffer(0)]],
    constant SolidUniforms &u [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    int x = int(gid.x) + u.renderX1;
    int y = int(gid.y) + u.renderY1;

    if (x >= u.renderX2 || y >= u.renderY2) {
        return;
    }

    int lx = x - u.outputX1;
    int ly = y - u.outputY1;
    int idx = ly * u.outputRowFloats + lx * 4;

    output[idx + 0] = 1.0;
    output[idx + 1] = 0.0;
    output[idx + 2] = 1.0;
    output[idx + 3] = 1.0;
}
)metal";

std::mutex gPipelineMutex;
std::unordered_map<id<MTLCommandQueue>, id<MTLComputePipelineState>> gPipelineCache;

id<MTLComputePipelineState> pipelineForQueue(id<MTLCommandQueue> queue) {
  std::lock_guard<std::mutex> lock(gPipelineMutex);
  const auto it = gPipelineCache.find(queue);
  if (it != gPipelineCache.end()) {
    return it->second;
  }

  NSError *error = nil;
  id<MTLLibrary> library = [queue.device newLibraryWithSource:@(kSolidKernelSource)
                                                       options:nil
                                                         error:&error];
  if (!library) {
    return nil;
  }

  id<MTLFunction> fn = [library newFunctionWithName:@"rimell_solid_colour"];
  if (!fn) {
    [library release];
    return nil;
  }

  id<MTLComputePipelineState> pipeline = [queue.device newComputePipelineStateWithFunction:fn
                                                                                      error:&error];
  [fn release];
  [library release];
  if (!pipeline) {
    return nil;
  }

  gPipelineCache[queue] = pipeline;
  return pipeline;
}

} // namespace

OfxStatus renderMetalSolidColour(const RenderContext &ctx) {
  if (!ctx.commandQueue || !ctx.outputData) {
    return kOfxStatErrValue;
  }
  if (ctx.renderX2 <= ctx.renderX1 || ctx.renderY2 <= ctx.renderY1 || ctx.outputRowFloats <= 0) {
    return kOfxStatErrValue;
  }

  id<MTLCommandQueue> queue = reinterpret_cast<id<MTLCommandQueue>>(ctx.commandQueue);
  id<MTLComputePipelineState> pipeline = pipelineForQueue(queue);
  if (!pipeline) {
    return kOfxStatGPURenderFailed;
  }

  SolidUniforms u{
      ctx.renderX1,
      ctx.renderY1,
      ctx.renderX2,
      ctx.renderY2,
      ctx.outputX1,
      ctx.outputY1,
      ctx.outputRowFloats,
  };

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
  id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
  [encoder setComputePipelineState:pipeline];
  [encoder setBuffer:reinterpret_cast<id<MTLBuffer>>(ctx.outputData) offset:0 atIndex:0];
  [encoder setBytes:&u length:sizeof(u) atIndex:1];

  const NSUInteger threadWidth = pipeline.threadExecutionWidth;
  const NSUInteger threadHeight =
      std::max<NSUInteger>(1, pipeline.maxTotalThreadsPerThreadgroup / threadWidth);
  const MTLSize threadsPerGroup = MTLSizeMake(threadWidth, threadHeight, 1);
  const int width = ctx.renderX2 - ctx.renderX1;
  const int height = ctx.renderY2 - ctx.renderY1;
  const MTLSize groups = MTLSizeMake((width + static_cast<int>(threadWidth) - 1) / threadWidth,
                                     (height + static_cast<int>(threadHeight) - 1) / threadHeight,
                                     1);

  [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threadsPerGroup];
  [encoder endEncoding];
  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];

  return commandBuffer.status == MTLCommandBufferStatusError ? kOfxStatGPURenderFailed : kOfxStatOK;
}

#endif
