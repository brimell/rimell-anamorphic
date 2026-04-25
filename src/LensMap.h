#pragma once

#include "Types.h"

namespace rimell {

struct LensMap {
  Vec2 sphericalCoord{};
  Vec2 anamorphicCoord{};
  Vec2 sampleCoord{};
  Vec2 caDirection{1.0f, 0.0f};
  float radius = 0.0f;
  float edgeMask = 0.0f;
  float centerProtectionMask = 0.0f;
  float transferAmount = 0.0f;
};

float lensIdentityAxisWarp(const RenderParams &params);
float lensIdentityBloomScale(const RenderParams &params);
float lensIdentityFlareScale(const RenderParams &params);
float lensIdentityGhostScaleX(const RenderParams &params);
float lensIdentityGhostScaleY(const RenderParams &params);

LensMap buildLensMap(float dstX, float dstY, const Image &source, int width, int height,
                     const RenderParams &params);
Vec2 lensMapToSourcePixel(const LensMap &map, const Image &source, int width, int height);
Vec2 applyEdgeCrop(float dstX, float dstY, const Image &source, float cropScale);
float automaticEdgeCropScale(const Image &source, int width, int height, const RenderParams &params);

} // namespace rimell
