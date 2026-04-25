#include "LensMap.h"

#include "MathUtils.h"

#include <algorithm>
#include <cmath>

namespace rimell {
namespace {

float safeSqueezeDelta(const RenderParams &params) {
  return std::max(0.0f, params.squeezeRatio - 1.0f);
}

float presetAxisWarp(int lensIdentity) {
  switch (lensIdentity) {
  case 1:
    return 0.22f;
  case 2:
    return 0.62f;
  case 3:
    return 0.46f;
  default:
    return 0.0f;
  }
}

float presetBloomScale(int lensIdentity) {
  switch (lensIdentity) {
  case 1:
    return 1.08f;
  case 2:
    return 1.45f;
  case 3:
    return 1.32f;
  default:
    return 1.0f;
  }
}

float presetFlareScale(int lensIdentity) {
  switch (lensIdentity) {
  case 1:
    return 1.08f;
  case 2:
    return 1.55f;
  case 3:
    return 1.25f;
  default:
    return 1.0f;
  }
}

float presetGhostScaleX(int lensIdentity) {
  switch (lensIdentity) {
  case 2:
    return 1.18f;
  case 3:
    return 1.1f;
  default:
    return 1.04f;
  }
}

float presetGhostScaleY(int lensIdentity) {
  switch (lensIdentity) {
  case 2:
    return 0.9f;
  case 3:
    return 0.94f;
  default:
    return 0.98f;
  }
}

} // namespace

float lensIdentityAxisWarp(const RenderParams &params) {
  return clamp01(params.axisWarp + presetAxisWarp(params.lensIdentity));
}

float lensIdentityBloomScale(const RenderParams &params) {
  return presetBloomScale(params.lensIdentity) * (1.0f + safeSqueezeDelta(params) * 0.35f);
}

float lensIdentityFlareScale(const RenderParams &params) {
  return presetFlareScale(params.lensIdentity) * (1.0f + safeSqueezeDelta(params) * 0.55f);
}

float lensIdentityGhostScaleX(const RenderParams &params) {
  return presetGhostScaleX(params.lensIdentity) * (1.0f + safeSqueezeDelta(params) * 0.12f);
}

float lensIdentityGhostScaleY(const RenderParams &params) {
  return presetGhostScaleY(params.lensIdentity) / (1.0f + safeSqueezeDelta(params) * 0.06f);
}

LensMap buildLensMap(float dstX, float dstY, const Image &source, int width, int height,
                     const RenderParams &params) {
  // Rimell Anamorphic does not literally desqueeze spherical footage. It builds
  // a synthetic anamorphic view map, then derives geometry, edge behaviour,
  // chromatic separation, highlight bloom, flare, and ghosting from that shared
  // virtual lens mapping.
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float halfW = std::max(1.0f, static_cast<float>(width) * 0.5f);
  const float halfH = std::max(1.0f, static_cast<float>(height) * 0.5f);
  const float aspect = halfW / halfH;

  LensMap map;
  map.sphericalCoord = {(dstX - cx) / halfW, (dstY - cy) / halfH};

  Vec2 spherical = map.sphericalCoord;
  const float breathe = 1.0f + params.breathingAmount * (0.5f - params.focusDistance) * params.breathingScale;
  spherical.x /= std::max(0.01f, breathe);
  spherical.y /= std::max(0.01f, breathe);

  const float viewedX = spherical.x * aspect;
  const float viewedY = spherical.y;
  map.radius = std::sqrt(viewedX * viewedX + viewedY * viewedY);

  const float centerRadius = 0.28f + clamp01(params.centerProtection) * 0.42f;
  const float centerFeather = 0.2f;
  const float warpMask = smoothstep(centerRadius, centerRadius + centerFeather, map.radius);
  map.centerProtectionMask = 1.0f - warpMask;
  const bool realAnamorphicUtility = params.inputMode == 1;
  const bool useUtilityGeometry = params.inputMode == 1 || params.inputMode == 2;
  map.transferAmount = useUtilityGeometry ? 1.0f : clamp01(params.anamorphicTransfer) * warpMask;
  map.edgeMask = smoothstep(std::max(0.0f, params.edgeCompressionStart), 1.0f, std::abs(spherical.x));

  const float axisWarp = realAnamorphicUtility ? clamp01(params.axisWarp) : lensIdentityAxisWarp(params);
  const float squeezeShape = 1.0f + safeSqueezeDelta(params) * (0.6f + axisWarp * 0.7f);
  const float axisY = spherical.y / std::max(0.1f, squeezeShape);
  const float axisRadius2 = spherical.x * spherical.x + axisY * axisY;
  const float axisRadius4 = axisRadius2 * axisRadius2;
  const float axisDelta = safeSqueezeDelta(params);

  const float scaleX = 1.0f + (params.barrel + axisWarp * axisDelta * 0.18f) * axisRadius2 +
                       (params.mustache + axisWarp * 0.035f) * axisRadius4;
  const float scaleY = 1.0f + (params.barrel - axisWarp * axisDelta * 0.07f) * axisRadius2 +
                       params.mustache * axisRadius4;

  Vec2 anamorphic{spherical.x * scaleX, spherical.y * scaleY};
  anamorphic.y *= 1.0f - params.verticalCompensation * axisRadius2 * params.verticalCompensationScale;

  if (useUtilityGeometry) {
    const float ratio = std::max(0.1f, params.squeezeRatio);
    if (params.squeezeMode == 1) {
      anamorphic.x *= ratio;
    } else if (params.squeezeMode == 2) {
      anamorphic.x /= ratio;
    }
  } else {
    const float edge = smoothstep(std::max(0.0f, params.edgeCompressionStart), 1.0f, std::abs(spherical.x));
    const float edgeCompression = (params.edgeCompression * params.edgeCompressionScale +
                                   axisWarp * axisDelta * 0.08f) *
                                  edge * edge * (1.0f - std::abs(spherical.y) * 0.25f);
    anamorphic.x -= (anamorphic.x < 0.0f ? -1.0f : 1.0f) * edgeCompression;
  }

  const float fovScale = 1.0f + params.horizontalFovBoost * (70.0f / std::max(10.0f, params.virtualFocalLength));
  anamorphic.x /= std::max(0.05f, fovScale);

  const float mumpsMask = std::exp(-(map.radius * map.radius) / 0.22f);
  const float mumps = (params.closeFocusMumps - params.faceWidthCompensation) *
                      smoothstep(1.0f, 0.0f, params.focusDistance) * params.mumpsScale * mumpsMask;
  anamorphic.x /= std::max(0.1f, 1.0f + mumps * (1.0f - clamp01(params.centerProtection) * 0.35f));

  map.anamorphicCoord = anamorphic;
  map.sampleCoord = {
      lerp(spherical.x, anamorphic.x, map.transferAmount),
      lerp(spherical.y, anamorphic.y, map.transferAmount),
  };

  Vec2 delta{map.sampleCoord.x - map.sphericalCoord.x, map.sampleCoord.y - map.sphericalCoord.y};
  const float deltaLength = std::sqrt(delta.x * delta.x + delta.y * delta.y);
  if (deltaLength > 0.00001f) {
    map.caDirection = {delta.x / deltaLength, delta.y / deltaLength};
  } else if (map.radius > 0.00001f) {
    map.caDirection = {viewedX / map.radius, viewedY / map.radius};
  }

  return map;
}

Vec2 lensMapToSourcePixel(const LensMap &map, const Image &source, int width, int height) {
  const float cx = static_cast<float>(source.bounds.x1 + source.bounds.x2 - 1) * 0.5f;
  const float cy = static_cast<float>(source.bounds.y1 + source.bounds.y2 - 1) * 0.5f;
  const float halfW = std::max(1.0f, static_cast<float>(width) * 0.5f);
  const float halfH = std::max(1.0f, static_cast<float>(height) * 0.5f);
  return {cx + map.sampleCoord.x * halfW, cy + map.sampleCoord.y * halfH};
}

} // namespace rimell
