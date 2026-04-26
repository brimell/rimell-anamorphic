#include "MetalRender.h"

#include "Diagnostics.h"

#import <Metal/Metal.h>

#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace rimell {
namespace {

struct MetalParams {
  float mix;
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
  int sourceRowFloats;
  int outputX1;
  int outputY1;
  int outputRowFloats;
  int renderX1;
  int renderY1;
  int renderX2;
  int renderY2;
};

MetalParams packParams(const RenderParams &params) {
  return {
      params.mix,
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
  };
}

const char *kernelSource = R"metal(
#include <metal_stdlib>
using namespace metal;

struct P {
  float mix; int debugView; int renderQuality; int inputMode; int squeezeMode; float anamorphicTransfer; int lensIdentity;
  float squeezeRatio; float axisWarp; float centerProtection; float edgeCompressionStart; float horizontalFovBoost;
  float virtualFocalLength; float breathingScale; float bokehStretch; float bokehRotation; float bokehEdgeFalloff;
  float bokehStretchScale; float bloomPixelScale; float bloomThresholdScale; int bloomRings; int bloomSamplesPerRing;
  float bloomEdgeKeepScale; float bloomVeilScale; float bloomCreamScale; float flareIntensity; float flareLength;
  float flareColourR; float flareColourG; float flareColourB; float flareThreshold; float flareAngle;
  float flareStepDensity; float flareSpanScale; float flareFalloff; float veil; float bloomRadius;
  float highlightCream; float blackLiftProtection; int ghostCount; float ghostSpread; float ghostTintR;
  float ghostTintG; float ghostTintB; float ghostIntensity; int coatingStyle; float coatingWarmResponse;
  float coatingCoolResponse; float edgeBlur; float tangentialSmear; float radialFalloff; float edgeBlurPixels;
  float fieldCurvaturePixels; float smearPixels; float barrel; float mustache; float verticalCompensation;
  float verticalCompensationScale; float closeFocusMumps; float faceWidthCompensation; float focusDistance;
  float breathingAmount; float mumpsScale; float lateralCA; float longitudinalCA; float edgeOnlyCA;
  float lateralCAPixelScale; float ovalVignette; float vignetteAsymmetry; float cornerBias; float ovalVignetteScale;
  float vignetteAsymmetryScale; float horizontalSmear; float verticalSharpness; float fieldCurvature;
  float catEyeStrength; float bokehVignette; float edgeCompression; float catEyeDimScale; float bokehVignetteDimScale;
  float edgeCompressionScale; float centerVeilScale; int guidesEnabled; int outputAspect; float customOutputAspect;
  float safeArea; int letterboxPreview; float letterboxOpacity; float guideAspectStrength; float guideSafeStrength;
  int autoEdgeCrop; float edgeCropScale;
};

struct I {
  int sourceX1; int sourceY1; int sourceX2; int sourceY2; int outputX1; int outputY1;
  int sourceRowFloats; int outputRowFloats; int width; int height; int renderX1; int renderY1; int renderX2; int renderY2;
};

struct LensMap { float2 sphericalCoord; float2 sampleCoord; float2 caDirection; float radius; float edgeMask; float transferAmount; };

constant float kPi = 3.14159265358979323846f;

float clamp01(float v) { return clamp(v, 0.0f, 1.0f); }
float finiteOr(float v, float fallback) { return isfinite(v) ? v : fallback; }
float2 finiteOr2(float2 v, float2 fallback) {
  return float2(finiteOr(v.x, fallback.x), finiteOr(v.y, fallback.y));
}
float lerpf(float a, float b, float t) { return a + (b - a) * t; }
float4 lerp4(float4 a, float4 b, float t) { return a + (b - a) * t; }
float luminance(float4 p) { return 0.2126f * p.x + 0.7152f * p.y + 0.0722f * p.z; }
float smoothstepf(float e0, float e1, float x) {
  if (e0 == e1) { return x < e0 ? 0.0f : 1.0f; }
  float t = clamp01((x - e0) / (e1 - e0));
  return t * t * (3.0f - 2.0f * t);
}
float safeSqueezeDelta(constant P& p) { return max(0.0f, p.squeezeRatio - 1.0f); }
// Keep these values in sync with LensIdentity.h. The Metal source is compiled
// from a runtime string, so the C++ inline helpers cannot be included directly.
float presetAxisWarp(int v) { return v == 1 ? 0.22f : (v == 2 ? 0.62f : (v == 3 ? 0.46f : 0.0f)); }
float presetBloomScale(int v) { return v == 1 ? 1.08f : (v == 2 ? 1.45f : (v == 3 ? 1.32f : 1.0f)); }
float presetFlareScale(int v) { return v == 1 ? 1.08f : (v == 2 ? 1.55f : (v == 3 ? 1.25f : 1.0f)); }
float presetGhostScaleX(int v) { return v == 2 ? 1.18f : (v == 3 ? 1.1f : 1.04f); }
float presetGhostScaleY(int v) { return v == 2 ? 0.9f : (v == 3 ? 0.94f : 0.98f); }
float lensIdentityAxisWarp(constant P& p) { return clamp01(p.axisWarp + presetAxisWarp(p.lensIdentity)); }
float lensIdentityBloomScale(constant P& p) { return presetBloomScale(p.lensIdentity) * (1.0f + safeSqueezeDelta(p) * 0.35f); }
float lensIdentityFlareScale(constant P& p) { return presetFlareScale(p.lensIdentity) * (1.0f + safeSqueezeDelta(p) * 0.55f); }
float lensIdentityGhostScaleX(constant P& p) { return presetGhostScaleX(p.lensIdentity) * (1.0f + safeSqueezeDelta(p) * 0.12f); }
float lensIdentityGhostScaleY(constant P& p) { return presetGhostScaleY(p.lensIdentity) / (1.0f + safeSqueezeDelta(p) * 0.06f); }

float4 loadPixel(const device float* src, constant I& info, int x, int y) {
  int cx = clamp(x, info.sourceX1, info.sourceX2 - 1) - info.sourceX1;
  int cy = clamp(y, info.sourceY1, info.sourceY2 - 1) - info.sourceY1;
  int idx = cy * info.sourceRowFloats + cx * 4;
  return float4(src[idx], src[idx + 1], src[idx + 2], src[idx + 3]);
}

float4 sampleNearest(const device float* src, constant I& info, float x, float y) {
  return loadPixel(src, info, int(floor(x + 0.5f)), int(floor(y + 0.5f)));
}

float4 sampleBilinear(const device float* src, constant I& info, float x, float y) {
  if (!isfinite(x) || !isfinite(y)) {
    return loadPixel(src, info, info.sourceX1, info.sourceY1);
  }
  int x0 = int(floor(x));
  int y0 = int(floor(y));
  float tx = x - float(x0);
  float ty = y - float(y0);
  float4 p00 = loadPixel(src, info, x0, y0);
  float4 p10 = loadPixel(src, info, x0 + 1, y0);
  float4 p01 = loadPixel(src, info, x0, y0 + 1);
  float4 p11 = loadPixel(src, info, x0 + 1, y0 + 1);
  return mix(mix(p00, p10, tx), mix(p01, p11, tx), ty);
}

LensMap buildLensMap(float dstX, float dstY, constant I& info, constant P& p) {
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float halfW = max(1.0f, float(info.width) * 0.5f);
  float halfH = max(1.0f, float(info.height) * 0.5f);
  float aspect = halfW / halfH;
  LensMap m;
  m.sphericalCoord = float2((dstX - cx) / halfW, (dstY - cy) / halfH);
  m.sphericalCoord = finiteOr2(m.sphericalCoord, float2(0.0f, 0.0f));
  float2 spherical = m.sphericalCoord;
  float breathe = 1.0f + p.breathingAmount * (0.5f - p.focusDistance) * p.breathingScale;
  spherical /= max(0.01f, breathe);
  spherical = finiteOr2(spherical, m.sphericalCoord);
  float viewedX = spherical.x * aspect;
  float viewedY = spherical.y;
  m.radius = sqrt(viewedX * viewedX + viewedY * viewedY);
  m.radius = finiteOr(m.radius, 0.0f);
  float centerRadius = 0.28f + clamp01(p.centerProtection) * 0.42f;
  float warpMask = smoothstepf(centerRadius, centerRadius + 0.2f, m.radius);
  bool realAnamorphicUtility = p.inputMode == 1;
  bool useUtilityGeometry = p.inputMode == 1 || p.inputMode == 2;
  m.transferAmount = useUtilityGeometry ? 1.0f : clamp01(p.anamorphicTransfer) * warpMask;
  m.edgeMask = smoothstepf(max(0.0f, p.edgeCompressionStart), 1.0f, abs(spherical.x));
  float axisWarp = realAnamorphicUtility ? clamp01(p.axisWarp) : lensIdentityAxisWarp(p);
  float axisDelta = safeSqueezeDelta(p);
  float squeezeShape = 1.0f + axisDelta * (0.6f + axisWarp * 0.7f);
  float axisY = spherical.y / max(0.1f, squeezeShape);
  float axisRadius2 = spherical.x * spherical.x + axisY * axisY;
  float axisRadius4 = axisRadius2 * axisRadius2;
  float scaleX = 1.0f + (p.barrel + axisWarp * axisDelta * 0.18f) * axisRadius2 + (p.mustache + axisWarp * 0.035f) * axisRadius4;
  float scaleY = 1.0f + (p.barrel - axisWarp * axisDelta * 0.07f) * axisRadius2 + p.mustache * axisRadius4;
  float2 anamorphic = float2(spherical.x * scaleX, spherical.y * scaleY);
  anamorphic = finiteOr2(anamorphic, spherical);
  anamorphic.y *= 1.0f - p.verticalCompensation * axisRadius2 * p.verticalCompensationScale;
  if (useUtilityGeometry) {
    float ratio = max(0.1f, p.squeezeRatio);
    if (p.squeezeMode == 1) anamorphic.x *= ratio;
    else if (p.squeezeMode == 2) anamorphic.x /= ratio;
  } else {
    float edge = smoothstepf(max(0.0f, p.edgeCompressionStart), 1.0f, abs(spherical.x));
    float edgeCompression = (p.edgeCompression * p.edgeCompressionScale + axisWarp * axisDelta * 0.08f) * edge * edge * (1.0f - abs(spherical.y) * 0.25f);
    anamorphic.x -= (anamorphic.x < 0.0f ? -1.0f : 1.0f) * edgeCompression;
  }
  float fovScale = 1.0f + p.horizontalFovBoost * (70.0f / max(10.0f, p.virtualFocalLength));
  anamorphic.x /= max(0.05f, fovScale);
  float mumpsMask = exp(-(m.radius * m.radius) / 0.22f);
  float mumps = (p.closeFocusMumps - p.faceWidthCompensation) * smoothstepf(1.0f, 0.0f, p.focusDistance) * p.mumpsScale * mumpsMask;
  anamorphic.x /= max(0.1f, 1.0f + mumps * (1.0f - clamp01(p.centerProtection) * 0.35f));
  m.sampleCoord = float2(lerpf(spherical.x, anamorphic.x, m.transferAmount), lerpf(spherical.y, anamorphic.y, m.transferAmount));
  m.sampleCoord = finiteOr2(m.sampleCoord, m.sphericalCoord);
  float2 delta = m.sampleCoord - m.sphericalCoord;
  float dl = length(delta);
  if (dl > 0.00001f) m.caDirection = delta / dl;
  else if (m.radius > 0.00001f) m.caDirection = float2(viewedX, viewedY) / m.radius;
  else m.caDirection = float2(1.0f, 0.0f);
  return m;
}

float2 lensMapToSourcePixel(LensMap m, constant I& info) {
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float2 pixel = float2(cx + m.sampleCoord.x * max(1.0f, float(info.width) * 0.5f),
                        cy + m.sampleCoord.y * max(1.0f, float(info.height) * 0.5f));
  return finiteOr2(pixel, float2(cx, cy));
}

float2 applyEdgeCrop(float dstX, float dstY, constant I& info, constant P& p) {
  float scale = max(1.0f, p.edgeCropScale);
  if (scale <= 1.0001f) return float2(dstX, dstY);
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  return float2(cx + (dstX - cx) / scale, cy + (dstY - cy) / scale);
}

float4 warpedSourceSample(const device float* src, constant I& info, constant P& p, float dstX, float dstY, float caPixels) {
  float2 cropped = applyEdgeCrop(dstX, dstY, info, p);
  LensMap lm = buildLensMap(cropped.x, cropped.y, info, p);
  float2 sp = lensMapToSourcePixel(lm, info);
  float caMask = p.edgeOnlyCA > 0.5f ? smoothstepf(0.2f, 1.0f, lm.radius) : 1.0f;
  sp += lm.caDirection * caPixels * caMask;
  return sampleBilinear(src, info, sp.x, sp.y);
}

float qualityScale(constant P& p) { return p.renderQuality == 0 ? 0.5f : (p.renderQuality == 2 ? 1.5f : 1.0f); }

float4 channelDefocusSample(const device float* src, constant I& info, constant P& p, float x, float y, float radiusPixels) {
  if (radiusPixels <= 0.05f) return warpedSourceSample(src, info, p, x, y, 0.0f);
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float2 d = float2(x - cx, y - cy);
  float len = length(d);
  d = len > 0.0001f ? d / len : float2(1.0f, 0.0f);
  float4 center = warpedSourceSample(src, info, p, x, y, 0.0f);
  float4 outward = warpedSourceSample(src, info, p, x + d.x * radiusPixels, y + d.y * radiusPixels, 0.0f);
  float4 inward = warpedSourceSample(src, info, p, x - d.x * radiusPixels, y - d.y * radiusPixels, 0.0f);
  return float4(center.xyz * 0.5f + (outward.xyz + inward.xyz) * 0.25f, center.w);
}

float4 opticalBaseSample(const device float* src, constant I& info, constant P& p, float x, float y) {
  float caPixels = p.lateralCA * p.lateralCAPixelScale;
  float4 base = warpedSourceSample(src, info, p, x, y, 0.0f);
  base.x = warpedSourceSample(src, info, p, x, y, caPixels).x;
  base.z = warpedSourceSample(src, info, p, x, y, -caPixels).z;
  if (p.longitudinalCA > 0.001f) {
    float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
    float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
    float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f);
    float ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
    float edge = smoothstepf(0.15f, 1.05f, sqrt(nx * nx + ny * ny));
    float focusBias = 0.35f + abs(p.focusDistance - 0.5f) * 1.3f;
    float radiusPixels = p.longitudinalCA * focusBias * edge * 3.0f;
    float amount = clamp01(p.longitudinalCA * (0.35f + edge * 0.65f));
    base.x = lerpf(base.x, channelDefocusSample(src, info, p, x, y, radiusPixels).x, amount);
    base.z = lerpf(base.z, channelDefocusSample(src, info, p, x, y, radiusPixels * 0.65f).z, amount);
  }
  return base;
}

float4 edgeCharacter(const device float* src, constant I& info, constant P& p, float x, float y, float4 base) {
  const int blurSteps = 7;
  const int smearSteps = 9;
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f);
  float ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
  LensMap lm = buildLensMap(x, y, info, p);
  float radius = max(lm.radius, sqrt(nx * nx + ny * ny));
  float edge = max(lm.edgeMask, smoothstepf(max(0.0f, 1.0f - p.radialFalloff), 1.15f, radius));
  float4 result = base;
  float blurRadius = p.edgeBlur * edge * p.edgeBlurPixels + p.fieldCurvature * edge * p.fieldCurvaturePixels;
  if (blurRadius > 0.05f) {
    float4 blur = 0.0f;
    float weight = 0.0f;
    for (int i = -blurSteps; i <= blurSteps; ++i) {
      float t = float(i) / float(blurSteps);
      float w = 1.0f - abs(t) * 0.55f;
      blur += warpedSourceSample(src, info, p, x + t * blurRadius, y + t * blurRadius * 0.25f, 0.0f) * w; weight += w;
    }
    result = lerp4(result, blur / weight, clamp01(edge * (p.edgeBlur + p.fieldCurvature)));
  }
  float smearRadius = edge * (p.tangentialSmear + p.horizontalSmear) * p.smearPixels;
  if (smearRadius > 0.05f) {
    float4 smear = 0.0f;
    float weight = 0.0f;
    for (int i = -smearSteps; i <= smearSteps; ++i) {
      float t = float(i) / float(smearSteps);
      float w = 1.0f - abs(t) * 0.7f;
      smear += warpedSourceSample(src, info, p, x + t * smearRadius, y, 0.0f) * w; weight += w;
    }
    result = lerp4(result, smear / weight, clamp01(edge * (p.tangentialSmear + p.horizontalSmear)));
  }
  if (p.verticalSharpness > 0.001f) {
    float4 up = warpedSourceSample(src, info, p, x, y - 1.5f, 0.0f);
    float4 down = warpedSourceSample(src, info, p, x, y + 1.5f, 0.0f);
    float sharpen = p.verticalSharpness * (1.0f - edge * 0.5f);
    result.y = clamp01(result.y + (result.y - (up.y + down.y) * 0.5f) * sharpen);
    result.x = clamp01(result.x + (result.x - (up.x + down.x) * 0.5f) * sharpen * 0.5f);
    result.z = clamp01(result.z + (result.z - (up.z + down.z) * 0.5f) * sharpen * 0.5f);
  }
  return result;
}

float highlightAt(const device float* src, constant I& info, constant P& p, float x, float y, float threshold) {
  return smoothstepf(threshold, 1.0f, luminance(warpedSourceSample(src, info, p, x, y, 0.0f)));
}

float4 lensAdditives(const device float* src, constant I& info, constant P& p, float x, float y, float4 base) {
  float4 add = 0.0f;
  float flareAngle = p.flareAngle * kPi / 180.0f;
  float dirX = cos(flareAngle), dirY = sin(flareAngle);
  float sampleScale = qualityScale(p);
  int flareSteps = max(1, int(round((2.0f + p.flareLength * p.flareStepDensity) * sampleScale)));
  float flareSpan = p.flareLength * float(info.width) * p.flareSpanScale * lensIdentityFlareScale(p);
  if (p.flareIntensity > 0.001f && flareSpan > 1.0f) {
    for (int i = -flareSteps; i <= flareSteps; ++i) {
      if (i == 0) continue;
      float t = float(i) / float(flareSteps);
      float h = highlightAt(src, info, p, x + dirX * t * flareSpan, y + dirY * t * flareSpan, p.flareThreshold);
      float w = exp(-abs(t) * p.flareFalloff) * h * p.flareIntensity;
      add.x += p.flareColourR * w; add.y += p.flareColourG * w; add.z += p.flareColourB * w;
    }
  }
  float bloomPixels = p.bloomRadius * p.bloomPixelScale;
  if ((p.veil > 0.001f || p.highlightCream > 0.001f) && bloomPixels > 0.5f) {
    float rotation = p.bokehRotation * kPi / 180.0f;
    float cosR = cos(rotation), sinR = sin(rotation);
    float stretch = (1.0f + p.bokehStretch * p.bokehStretchScale) * lensIdentityBloomScale(p);
    int rings = max(1, int(round(float(p.bloomRings) * sampleScale)));
    int samplesPerRing = max(3, int(round(float(p.bloomSamplesPerRing) * sampleScale)));
    float total = 0.0f; float4 bloom = 0.0f;
    for (int ring = 1; ring <= rings; ++ring) {
      float ringRadius = bloomPixels * float(ring) / float(rings);
      for (int i = 0; i < samplesPerRing; ++i) {
        float a = 2.0f * kPi * float(i) / float(samplesPerRing);
        float ox = cos(a) * ringRadius / stretch;
        float oy = sin(a) * ringRadius * stretch;
        float4 s = warpedSourceSample(src, info, p, x + ox * cosR - oy * sinR, y + ox * sinR + oy * cosR, 0.0f);
        float h = smoothstepf(p.flareThreshold * p.bloomThresholdScale, 1.0f, luminance(s));
        float w = h / float(ring);
        bloom += s * w; total += w;
      }
    }
    if (total > 0.0f) {
      bloom /= total;
      LensMap lm = buildLensMap(x, y, info, p);
      float edge = max(lm.edgeMask, smoothstepf(0.35f, 1.1f, lm.radius));
      float bokehEdgeKeep = 1.0f - edge * p.bokehEdgeFalloff * p.bloomEdgeKeepScale;
      float protect = lerpf(1.0f, clamp01(luminance(base) * 2.0f), p.blackLiftProtection);
      float amount = (p.veil * p.bloomVeilScale + p.highlightCream * p.bloomCreamScale) * protect * bokehEdgeKeep;
      add.xyz += bloom.xyz * amount;
    }
  }
  if (p.ghostCount > 0 && p.ghostSpread > 0.001f) {
    float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
    float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
    float tintShift = p.coatingStyle == 0 ? p.coatingWarmResponse : (p.coatingStyle == 2 ? p.coatingCoolResponse : 1.0f);
    int ghostCount = p.renderQuality == 0 ? min(p.ghostCount, 3) : (p.renderQuality == 2 ? p.ghostCount : min(p.ghostCount, 6));
    for (int i = 1; i <= ghostCount; ++i) {
      float scale = 1.0f + p.ghostSpread * float(i);
      float4 g = warpedSourceSample(src, info, p, cx - (x - cx) * scale * lensIdentityGhostScaleX(p), cy - (y - cy) * scale * lensIdentityGhostScaleY(p), 0.0f);
      float w = smoothstepf(p.flareThreshold, 1.0f, luminance(g)) * (p.ghostIntensity / float(i)) * tintShift;
      add.x += g.x * p.ghostTintR * w; add.y += g.y * p.ghostTintG * w; add.z += g.z * p.ghostTintB * w;
    }
  }
  float centerGlow = smoothstepf(p.flareThreshold * 0.9f, 1.0f, luminance(base));
  add.xyz += p.veil * centerGlow * p.centerVeilScale;
  return add;
}

float aspectValue(int index, float customOutputAspect) {
  return index == 0 ? 2.0f : (index == 1 ? 2.39f : (index == 2 ? 2.66f : max(0.1f, customOutputAspect)));
}

float3 applyVignetteCatEye(float3 rgb, float x, float y, constant I& info, constant P& p) {
  float cx = (float(info.width) - 1.0f) * 0.5f;
  float cy = (float(info.height) - 1.0f) * 0.5f;
  float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f);
  float ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
  nx = finiteOr(nx, 0.0f);
  ny = finiteOr(ny, 0.0f);

  float ovalY = ny * (1.0f + p.ovalVignette * p.ovalVignetteScale);
  float asym = nx * p.vignetteAsymmetry * p.vignetteAsymmetryScale +
               ny * p.cornerBias * p.vignetteAsymmetryScale;
  float vignetteShape = sqrt(nx * nx + ovalY * ovalY) + asym;
  float vignette = 1.0f - p.ovalVignette * smoothstepf(0.35f, 1.2f, vignetteShape);

  float edge = smoothstepf(0.55f, 1.08f, sqrt(nx * nx + ny * ny));
  float catEyeDim = p.catEyeStrength * edge * p.catEyeDimScale;

  float dim = max(0.0f, vignette * (1.0f - catEyeDim));
  return rgb * dim;
}

float4 applyVignetteAndGuides(float4 color, float x, float y, constant I& info, constant P& p) {
  float cx = (float(info.width) - 1.0f) * 0.5f, cy = (float(info.height) - 1.0f) * 0.5f;
  float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f), ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
  float ovalY = ny * (1.0f + p.ovalVignette * p.ovalVignetteScale);
  float asym = nx * p.vignetteAsymmetry * p.vignetteAsymmetryScale + ny * p.cornerBias * p.vignetteAsymmetryScale;
  float vignette = 1.0f - p.ovalVignette * smoothstepf(0.35f, 1.2f, sqrt(nx * nx + ovalY * ovalY) + asym);
  color.xyz *= vignette;
  float edge = smoothstepf(0.55f, 1.08f, sqrt(nx * nx + ny * ny));
  color.xyz *= 1.0f - p.catEyeStrength * edge * p.catEyeDimScale - p.bokehVignette * edge * p.bokehVignetteDimScale;
  if (p.guidesEnabled != 0 || p.letterboxPreview != 0) {
    float target = aspectValue(p.outputAspect, p.customOutputAspect);
    float current = float(info.width) / max(1.0f, float(info.height));
    float contentX1 = 0.0f, contentY1 = 0.0f, contentX2 = float(info.width), contentY2 = float(info.height);
    if (target > current) { float ch = float(info.width) / target; contentY1 = (float(info.height) - ch) * 0.5f; contentY2 = contentY1 + ch; }
    else { float cw = float(info.height) * target; contentX1 = (float(info.width) - cw) * 0.5f; contentX2 = contentX1 + cw; }
    bool outside = x < contentX1 || x >= contentX2 || y < contentY1 || y >= contentY2;
    if (p.letterboxPreview != 0 && outside) color.xyz *= 1.0f - clamp01(p.letterboxOpacity);
    if (p.guidesEnabled != 0) {
      float sx1 = contentX1 + (contentX2 - contentX1) * (1.0f - p.safeArea) * 0.5f;
      float sx2 = contentX2 - (contentX2 - contentX1) * (1.0f - p.safeArea) * 0.5f;
      float sy1 = contentY1 + (contentY2 - contentY1) * (1.0f - p.safeArea) * 0.5f;
      float sy2 = contentY2 - (contentY2 - contentY1) * (1.0f - p.safeArea) * 0.5f;
      bool aspectLine = abs(x - contentX1) < 1.0f || abs(x - contentX2) < 1.0f || abs(y - contentY1) < 1.0f || abs(y - contentY2) < 1.0f;
      bool safeLine = abs(x - sx1) < 1.0f || abs(x - sx2) < 1.0f || abs(y - sy1) < 1.0f || abs(y - sy2) < 1.0f;
      if (aspectLine || safeLine) {
        float strength = aspectLine ? p.guideAspectStrength : p.guideSafeStrength;
        color.x = lerpf(color.x, 1.0f, strength); color.y = lerpf(color.y, safeLine ? 0.85f : 0.65f, strength); color.z = lerpf(color.z, 0.25f, strength);
      }
    }
  }
  return color;
}

kernel void RimellAnamorphicKernel(const device float* src [[buffer(0)]], device float* dst [[buffer(1)]],
                                   constant P& p [[buffer(2)]], constant I& info [[buffer(3)]],
                                   uint2 gid [[thread_position_in_grid]]) {
  int x = info.renderX1 + int(gid.x);
  int y = info.renderY1 + int(gid.y);
  if (x >= info.renderX2 || y >= info.renderY2) return;
  int outIndex = (y - info.outputY1) * info.outputRowFloats + (x - info.outputX1) * 4;

  float4 original = sampleNearest(src, info, float(x), float(y));

  if (p.debugView == 4) {
    float h = smoothstepf(p.flareThreshold, 1.0f, luminance(original));
    dst[outIndex] = h;
    dst[outIndex + 1] = h;
    dst[outIndex + 2] = h;
    dst[outIndex + 3] = 1.0f;
    return;
  }

  float4 color = warpedSourceSample(src, info, p, float(x), float(y), 0.0f);
  float caPixels = p.lateralCA * p.lateralCAPixelScale;
  if (abs(caPixels) > 0.0001f) {
    color.x = warpedSourceSample(src, info, p, float(x), float(y), caPixels).x;
    color.z = warpedSourceSample(src, info, p, float(x), float(y), -caPixels).z;
  }

  float3 graded = applyVignetteCatEye(color.xyz, float(x - info.sourceX1), float(y - info.sourceY1), info, p);
  float4 stylized = float4(finiteOr(graded.x, original.x),
                           finiteOr(graded.y, original.y),
                           finiteOr(graded.z, original.z),
                           original.w);

  float blend = clamp01(p.mix);
  float4 outPixel = lerp4(original, stylized, blend);
  dst[outIndex] = outPixel.x;
  dst[outIndex + 1] = outPixel.y;
  dst[outIndex + 2] = outPixel.z;
  dst[outIndex + 3] = outPixel.w;
}
)metal";

const char *copyKernelSource = R"metal(
#include <metal_stdlib>
using namespace metal;

struct CopyUniforms {
  int sourceX1;
  int sourceY1;
  int sourceRowFloats;
  int outputX1;
  int outputY1;
  int outputRowFloats;
  int renderX1;
  int renderY1;
  int renderX2;
  int renderY2;
};

kernel void rimell_copy(
  device const float *source [[buffer(0)]],
  device float *output [[buffer(1)]],
  constant CopyUniforms &u [[buffer(2)]],
  uint2 gid [[thread_position_in_grid]]
) {
  int x = int(gid.x) + u.renderX1;
  int y = int(gid.y) + u.renderY1;

  if (x >= u.renderX2 || y >= u.renderY2) {
    return;
  }

  int sx = x - u.sourceX1;
  int sy = y - u.sourceY1;
  int ox = x - u.outputX1;
  int oy = y - u.outputY1;

  int sidx = sy * u.sourceRowFloats + sx * 4;
  int oidx = oy * u.outputRowFloats + ox * 4;

  output[oidx + 0] = source[sidx + 0];
  output[oidx + 1] = source[sidx + 1];
  output[oidx + 2] = source[sidx + 2];
  output[oidx + 3] = source[sidx + 3];
}
)metal";

std::mutex pipelineMutex;
std::unordered_map<id<MTLCommandQueue>, id<MTLComputePipelineState>> pipelineCache;

std::mutex copyPipelineMutex;
std::unordered_map<id<MTLCommandQueue>, id<MTLComputePipelineState>> copyPipelineCache;

id<MTLComputePipelineState> pipelineForQueue(id<MTLCommandQueue> queue) {
  std::lock_guard<std::mutex> lock(pipelineMutex);
  const auto it = pipelineCache.find(queue);
  if (it != pipelineCache.end()) {
    return it->second;
  }

  NSError *error = nil;
  MTLCompileOptions *options = [MTLCompileOptions new];
  id<MTLLibrary> library = [queue.device newLibraryWithSource:@(kernelSource) options:options error:&error];
  [options release];
  if (!library) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to compile metal library error=%s",
              error ? [[error localizedDescription] UTF8String] : "(none)");
    return nil;
  }

  id<MTLFunction> function = [library newFunctionWithName:@"RimellAnamorphicKernel"];
  if (!function) {
    logMessage(LogLevel::Error, "render.gpu", "failed to find RimellAnamorphicKernel function");
    [library release];
    return nil;
  }

  id<MTLComputePipelineState> pipeline = [queue.device newComputePipelineStateWithFunction:function error:&error];
  [function release];
  [library release];
  if (!pipeline) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to build compute pipeline error=%s",
              error ? [[error localizedDescription] UTF8String] : "(none)");
  }
  if (pipeline) {
    pipelineCache[queue] = pipeline;
  }
  return pipeline;
}

id<MTLComputePipelineState> copyPipelineForQueue(id<MTLCommandQueue> queue) {
  std::lock_guard<std::mutex> lock(copyPipelineMutex);
  const auto it = copyPipelineCache.find(queue);
  if (it != copyPipelineCache.end()) {
    return it->second;
  }

  NSError *error = nil;
  MTLCompileOptions *options = [MTLCompileOptions new];
  id<MTLLibrary> library = [queue.device newLibraryWithSource:@(copyKernelSource) options:options error:&error];
  [options release];
  if (!library) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to compile metal copy library error=%s",
              error ? [[error localizedDescription] UTF8String] : "(none)");
    return nil;
  }

  id<MTLFunction> function = [library newFunctionWithName:@"rimell_copy"];
  if (!function) {
    logMessage(LogLevel::Error, "render.gpu", "failed to find rimell_copy function");
    [library release];
    return nil;
  }

  id<MTLComputePipelineState> pipeline = [queue.device newComputePipelineStateWithFunction:function error:&error];
  [function release];
  [library release];
  if (!pipeline) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "failed to build copy pipeline error=%s",
              error ? [[error localizedDescription] UTF8String] : "(none)");
  }
  if (pipeline) {
    copyPipelineCache[queue] = pipeline;
  }
  return pipeline;
}

} // namespace

OfxStatus renderMetalFloat(void *commandQueue, const Image &source, const Image &output,
                           const OfxRectI &renderWindow, const RenderParams &params) {
  ScopedLogTimer timer(LogLevel::Info, "render.gpu", "renderMetalFloat");
  timer.setResult("in_progress");

  if (!commandQueue || !source.data || !output.data) {
    logMessage(LogLevel::Error, "render.gpu", "invalid queue/source/output pointers");
    timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
    return kOfxStatGPURenderFailed;
  }

  id<MTLCommandQueue> queue = reinterpret_cast<id<MTLCommandQueue>>(commandQueue);
  id<MTLComputePipelineState> pipeline = pipelineForQueue(queue);
  if (!pipeline) {
    timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
    return kOfxStatGPURenderFailed;
  }

  const int width = source.bounds.x2 - source.bounds.x1;
  const int height = source.bounds.y2 - source.bounds.y1;
  if (width <= 0 || height <= 0) {
    logPrintf(LogLevel::Error, "render.gpu", "invalid source bounds width=%d height=%d", width, height);
    timer.setResult(ofxStatusToString(kOfxStatErrValue));
    return kOfxStatErrValue;
  }

  logPrintf(LogLevel::Debug,
            "render.gpu",
            "dispatch source=[%d,%d,%d,%d] output=[%d,%d,%d,%d] window=[%d,%d,%d,%d]",
            source.bounds.x1,
            source.bounds.y1,
            source.bounds.x2,
            source.bounds.y2,
            output.bounds.x1,
            output.bounds.y1,
            output.bounds.x2,
            output.bounds.y2,
            renderWindow.x1,
            renderWindow.y1,
            renderWindow.x2,
            renderWindow.y2);

  MetalParams packedParams = packParams(params);
  MetalImageInfo info{
      source.bounds.x1,
      source.bounds.y1,
      source.bounds.x2,
      source.bounds.y2,
      output.bounds.x1,
      output.bounds.y1,
      source.rowBytes / static_cast<int>(sizeof(float)),
      output.rowBytes / static_cast<int>(sizeof(float)),
      width,
      height,
      renderWindow.x1,
      renderWindow.y1,
      renderWindow.x2,
      renderWindow.y2,
  };

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
  id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
  [encoder setComputePipelineState:pipeline];
  [encoder setBuffer:reinterpret_cast<id<MTLBuffer>>(source.data) offset:0 atIndex:0];
  [encoder setBuffer:reinterpret_cast<id<MTLBuffer>>(output.data) offset:0 atIndex:1];
  [encoder setBytes:&packedParams length:sizeof(packedParams) atIndex:2];
  [encoder setBytes:&info length:sizeof(info) atIndex:3];

  const NSUInteger threadWidth = pipeline.threadExecutionWidth;
  const NSUInteger threadHeight = std::max<NSUInteger>(1, pipeline.maxTotalThreadsPerThreadgroup / threadWidth);
  MTLSize threadsPerGroup = MTLSizeMake(threadWidth, threadHeight, 1);
  const int renderWidth = renderWindow.x2 - renderWindow.x1;
  const int renderHeight = renderWindow.y2 - renderWindow.y1;
  MTLSize groups = MTLSizeMake((renderWidth + static_cast<int>(threadWidth) - 1) / threadWidth,
                               (renderHeight + static_cast<int>(threadHeight) - 1) / threadHeight, 1);
  [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threadsPerGroup];
  [encoder endEncoding];
  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];

  const bool failed = commandBuffer.status == MTLCommandBufferStatusError;
  if (failed) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "command buffer failed error=%s",
              commandBuffer.error ? [[commandBuffer.error localizedDescription] UTF8String] : "(none)");
  }
  const OfxStatus status = failed ? kOfxStatGPURenderFailed : kOfxStatOK;
  timer.setResult(ofxStatusToString(status));
  return status;
}

OfxStatus renderMetalCopy(void *commandQueue, const Image &source, const Image &output,
                          const OfxRectI &renderWindow) {
  ScopedLogTimer timer(LogLevel::Info, "render.gpu", "renderMetalCopy");
  timer.setResult("in_progress");

  if (!commandQueue || !source.data || !output.data) {
    logMessage(LogLevel::Error, "render.gpu", "invalid queue/source/output pointers");
    timer.setResult(ofxStatusToString(kOfxStatGPURenderFailed));
    return kOfxStatGPURenderFailed;
  }

  id<MTLCommandQueue> queue = reinterpret_cast<id<MTLCommandQueue>>(commandQueue);
  id<MTLComputePipelineState> pipeline = copyPipelineForQueue(queue);
  if (!pipeline) {
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

  CopyUniforms uniforms{
      source.bounds.x1,
      source.bounds.y1,
      source.rowBytes / static_cast<int>(sizeof(float)),
      output.bounds.x1,
      output.bounds.y1,
      output.rowBytes / static_cast<int>(sizeof(float)),
      renderWindow.x1,
      renderWindow.y1,
      renderWindow.x2,
      renderWindow.y2,
  };

  id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
  id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
  [encoder setComputePipelineState:pipeline];
  [encoder setBuffer:reinterpret_cast<id<MTLBuffer>>(source.data) offset:0 atIndex:0];
  [encoder setBuffer:reinterpret_cast<id<MTLBuffer>>(output.data) offset:0 atIndex:1];
  [encoder setBytes:&uniforms length:sizeof(uniforms) atIndex:2];

  const NSUInteger threadWidth = pipeline.threadExecutionWidth;
  const NSUInteger threadHeight = std::max<NSUInteger>(1, pipeline.maxTotalThreadsPerThreadgroup / threadWidth);
  MTLSize threadsPerGroup = MTLSizeMake(threadWidth, threadHeight, 1);
  MTLSize groups = MTLSizeMake((renderWidth + static_cast<int>(threadWidth) - 1) / threadWidth,
                               (renderHeight + static_cast<int>(threadHeight) - 1) / threadHeight,
                               1);
  [encoder dispatchThreadgroups:groups threadsPerThreadgroup:threadsPerGroup];
  [encoder endEncoding];
  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];

  const bool failed = commandBuffer.status == MTLCommandBufferStatusError;
  if (failed) {
    logPrintf(LogLevel::Error,
              "render.gpu",
              "copy command buffer failed error=%s",
              commandBuffer.error ? [[commandBuffer.error localizedDescription] UTF8String] : "(none)");
  }
  const OfxStatus status = failed ? kOfxStatGPURenderFailed : kOfxStatOK;
  timer.setResult(ofxStatusToString(status));
  return status;
}

} // namespace rimell
