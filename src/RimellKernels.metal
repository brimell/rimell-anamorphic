#include <metal_stdlib>
using namespace metal;

typedef float PixelChannel;

float channelToFloat(PixelChannel v) {
  return v;
}

PixelChannel floatToChannel(float v) {
  return v;
}

constant int kUniformVersion = 1;

struct P {
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
  int enableBokeh;
  float bokehAmount;
  float focusWidth;
  float focusFalloff;
  float maxBokehRadius;
  float nearBlurAmount;
  float farBlurAmount;
  float ovalRatio;
  int ovalOrientation;
  float ovalAngle;
  int invertDepth;
  float depthBlackPoint;
  float depthWhitePoint;
  float depthGamma;
  int depthSmoothRadius;
  float depthEdgeProtect;
  float foregroundEdgeProtect;
  float backgroundEdgeProtect;
  float occlusionThreshold;
  int highlightBokehEnable;
  float highlightThreshold;
  float highlightSoftness;
  float highlightGain;
  float highlightRadiusMultiplier;
  float highlightSaturation;
  float highlightRolloff;
  float apertureSoftness;
  float rimBrightness;
  float centreDensity;
  int bokehCAEnable;
  float bokehCAAmount;
  float catEyeAmount;
  float catEyeStart;
  float catEyeCompression;
  float catEyeShift;
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
  int enableDepthMap;
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
  int enableHighlightEffects;
  int enableEdgeEffects;
  int enableAdditionalBackgroundBlur;
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

struct I {
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

  int depthX1;
  int depthY1;
  int depthX2;
  int depthY2;
  int depthRowFloats;
  int hasDepth;
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

struct LensMap {
  float2 sphericalCoord;
  float2 sampleCoord;
  float2 caDirection;
  float radius;
  float edgeMask;
  float transferAmount;
};

struct BasicLensMap {
  float2 sampleCoord;
};

constant float kPi = 3.14159265358979323846f;

float clamp01(float v) { return clamp(v, 0.0f, 1.0f); }
float finiteOr(float v, float fallback) { return isfinite(v) ? v : fallback; }
float safeFloatOut(float v, float fallback) {
  if (!isfinite(v)) {
    return fallback;
  }
  return clamp(v, -65504.0f, 65504.0f);
}
float2 finiteOr2(float2 v, float2 fallback) {
  return float2(finiteOr(v.x, fallback.x), finiteOr(v.y, fallback.y));
}
float lerpf(float a, float b, float t) { return a + (b - a) * t; }
float4 lerp4(float4 a, float4 b, float t) { return a + (b - a) * t; }
float luminance(float4 p) { return 0.2126f * p.x + 0.7152f * p.y + 0.0722f * p.z; }
float smoothstepf(float e0, float e1, float x) {
  if (e0 == e1) {
    return x < e0 ? 0.0f : 1.0f;
  }
  float t = clamp01((x - e0) / (e1 - e0));
  return t * t * (3.0f - 2.0f * t);
}
float safeSqueezeDelta(constant P &p) { return p.safeSqueezeDelta; }

void writePixel(device PixelChannel *dst, int index, float4 value, float4 fallback) {
  dst[index + 0] = safeFloatOut(value.x, fallback.x);
  dst[index + 1] = safeFloatOut(value.y, fallback.y);
  dst[index + 2] = safeFloatOut(value.z, fallback.z);
  dst[index + 3] = safeFloatOut(value.w, fallback.w);
}

float presetAxisWarp(int v) { return v == 1 ? 0.22f : (v == 2 ? 0.62f : (v == 3 ? 0.46f : 0.0f)); }
float lensIdentityAxisWarp(constant P &p) { return clamp01(p.axisWarp + presetAxisWarp(p.lensIdentity)); }
float lensIdentityBloomScale(constant P &p) { return p.lensIdentityBloomScale; }
float lensIdentityFlareScale(constant P &p) { return p.lensIdentityFlareScale; }
float lensIdentityGhostScaleX(constant P &p) { return p.lensIdentityGhostScaleX; }
float lensIdentityGhostScaleY(constant P &p) { return p.lensIdentityGhostScaleY; }

float4 loadPixel(const device PixelChannel *src, constant I &info, int x, int y) {
  int cx = clamp(x, info.sourceX1, info.sourceX2 - 1) - info.sourceX1;
  int cy = clamp(y, info.sourceY1, info.sourceY2 - 1) - info.sourceY1;
  int idx = cy * info.sourceRowFloats + cx * 4;
  return float4(channelToFloat(src[idx]),
                channelToFloat(src[idx + 1]),
                channelToFloat(src[idx + 2]),
                channelToFloat(src[idx + 3]));
}

float4 sampleNearest(const device PixelChannel *src, constant I &info, float x, float y) {
  return loadPixel(src, info, int(floor(x + 0.5f)), int(floor(y + 0.5f)));
}

float sampleDepthBilinear(const device PixelChannel *depth, constant I &info, float x, float y) {
  if (info.hasDepth == 0) return 0.5f;

  int x0 = int(floor(x));
  int y0 = int(floor(y));
  float tx = x - float(x0);
  float ty = y - float(y0);

  int cx0 = clamp(x0, info.depthX1, info.depthX2 - 1) - info.depthX1;
  int cx1 = clamp(x0 + 1, info.depthX1, info.depthX2 - 1) - info.depthX1;
  int cy0 = clamp(y0, info.depthY1, info.depthY2 - 1) - info.depthY1;
  int cy1 = clamp(y0 + 1, info.depthY1, info.depthY2 - 1) - info.depthY1;

  int row0 = cy0 * info.depthRowFloats;
  int row1 = cy1 * info.depthRowFloats;

  float p00 = depth[row0 + cx0];
  float p10 = depth[row0 + cx1];
  float p01 = depth[row1 + cx0];
  float p11 = depth[row1 + cx1];

  float p0 = mix(p00, p10, tx);
  float p1 = mix(p01, p11, tx);

  return mix(p0, p1, ty);
}

float4 sampleBilinear(const device PixelChannel *src, constant I &info, float x, float y) {
  int x0 = int(floor(x));
  int y0 = int(floor(y));
  float tx = x - float(x0);
  float ty = y - float(y0);

  int cx0 = clamp(x0, info.sourceX1, info.sourceX2 - 1) - info.sourceX1;
  int cx1 = clamp(x0 + 1, info.sourceX1, info.sourceX2 - 1) - info.sourceX1;
  int cy0 = clamp(y0, info.sourceY1, info.sourceY2 - 1) - info.sourceY1;
  int cy1 = clamp(y0 + 1, info.sourceY1, info.sourceY2 - 1) - info.sourceY1;

  int row0 = cy0 * info.sourceRowFloats;
  int row1 = cy1 * info.sourceRowFloats;

  int idx00 = row0 + cx0 * 4;
  int idx10 = row0 + cx1 * 4;
  int idx01 = row1 + cx0 * 4;
  int idx11 = row1 + cx1 * 4;

  float4 p00 = float4(channelToFloat(src[idx00]), channelToFloat(src[idx00+1]), channelToFloat(src[idx00+2]), channelToFloat(src[idx00+3]));
  float4 p10 = float4(channelToFloat(src[idx10]), channelToFloat(src[idx10+1]), channelToFloat(src[idx10+2]), channelToFloat(src[idx10+3]));
  float4 p01 = float4(channelToFloat(src[idx01]), channelToFloat(src[idx01+1]), channelToFloat(src[idx01+2]), channelToFloat(src[idx01+3]));
  float4 p11 = float4(channelToFloat(src[idx11]), channelToFloat(src[idx11+1]), channelToFloat(src[idx11+2]), channelToFloat(src[idx11+3]));

  return mix(mix(p00, p10, tx), mix(p01, p11, tx), ty);
}

float4 sampleBilinearZero(const device PixelChannel *src, constant I &info, float x, float y) {
  if (!isfinite(x) || !isfinite(y) ||
      x < float(info.sourceX1) || y < float(info.sourceY1) ||
      x > float(info.sourceX2 - 1) || y > float(info.sourceY2 - 1)) {
    return float4(0.0f);
  }
  return sampleBilinear(src, info, x, y);
}

struct LocalMapping {
  float2 spCenter;
  float2 spDx;
  float2 spDy;
  float2 caDirection;
};

float4 fastSourceSampleCA(const device PixelChannel *src, constant I &info, constant P &p, LocalMapping map, float dx, float dy, float caPixels) {
  float2 sp = map.spCenter + map.spDx * dx + map.spDy * dy + map.caDirection * caPixels;
  return sampleBilinear(src, info, sp.x, sp.y);
}

LensMap buildLensMap(float dstX, float dstY, constant I &info, constant P &p) {
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
    if (p.squeezeMode == 1) {
      anamorphic.x *= ratio;
    } else if (p.squeezeMode == 2) {
      anamorphic.x /= ratio;
    }
  } else {
    float edge = smoothstepf(max(0.0f, p.edgeCompressionStart), 1.0f, abs(spherical.x));
    float edgeCompression = (p.edgeCompression * p.edgeCompressionScale + axisWarp * axisDelta * 0.08f) * edge *
                            edge * (1.0f - abs(spherical.y) * 0.25f);
    anamorphic.x -= (anamorphic.x < 0.0f ? -1.0f : 1.0f) * edgeCompression;
  }
  float fovScale = 1.0f + p.horizontalFovBoost * (70.0f / max(10.0f, p.virtualFocalLength));
  anamorphic.x /= max(0.05f, fovScale);
  float mumpsMask = exp(-(m.radius * m.radius) / 0.22f);
  float mumps = (p.closeFocusMumps - p.faceWidthCompensation) * smoothstepf(1.0f, 0.0f, p.focusDistance) * p.mumpsScale *
                mumpsMask;
  anamorphic.x /= max(0.1f, 1.0f + mumps * (1.0f - clamp01(p.centerProtection) * 0.35f));
  m.sampleCoord = float2(lerpf(spherical.x, anamorphic.x, m.transferAmount), lerpf(spherical.y, anamorphic.y, m.transferAmount));
  m.sampleCoord = finiteOr2(m.sampleCoord, m.sphericalCoord);
  float2 delta = m.sampleCoord - m.sphericalCoord;
  float dl = length(delta);
  if (dl > 0.00001f) {
    m.caDirection = delta / dl;
  } else if (m.radius > 0.00001f) {
    m.caDirection = float2(viewedX, viewedY) / m.radius;
  } else {
    m.caDirection = float2(1.0f, 0.0f);
  }
  return m;
}

float2 lensMapToSourcePixel(LensMap m, constant I &info) {
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float2 pixel = float2(cx + m.sampleCoord.x * max(1.0f, float(info.width) * 0.5f),
                        cy + m.sampleCoord.y * max(1.0f, float(info.height) * 0.5f));
  return finiteOr2(pixel, float2(cx, cy));
}

float2 sampleCoordToSourcePixel(float2 sampleCoord, constant I &info) {
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float2 pixel = float2(cx + sampleCoord.x * max(1.0f, float(info.width) * 0.5f),
                        cy + sampleCoord.y * max(1.0f, float(info.height) * 0.5f));
  return finiteOr2(pixel, float2(cx, cy));
}

BasicLensMap buildBasicLensMap(float dstX, float dstY, constant I &info, constant P &p) {
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float halfW = max(1.0f, float(info.width) * 0.5f);
  float halfH = max(1.0f, float(info.height) * 0.5f);
  float aspect = halfW / halfH;
  float2 spherical = float2((dstX - cx) / halfW, (dstY - cy) / halfH);
  spherical = finiteOr2(spherical, float2(0.0f, 0.0f));
  float breathe = 1.0f + p.breathingAmount * (0.5f - p.focusDistance) * p.breathingScale;
  spherical /= max(0.01f, breathe);
  spherical = finiteOr2(spherical, float2(0.0f, 0.0f));
  float viewedX = spherical.x * aspect;
  float viewedY = spherical.y;
  float radius = finiteOr(sqrt(viewedX * viewedX + viewedY * viewedY), 0.0f);
  float centerRadius = 0.28f + clamp01(p.centerProtection) * 0.42f;
  float warpMask = smoothstepf(centerRadius, centerRadius + 0.2f, radius);
  bool realAnamorphicUtility = p.inputMode == 1;
  bool useUtilityGeometry = p.inputMode == 1 || p.inputMode == 2;
  float transferAmount = useUtilityGeometry ? 1.0f : clamp01(p.anamorphicTransfer) * warpMask;
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
    if (p.squeezeMode == 1) {
      anamorphic.x *= ratio;
    } else if (p.squeezeMode == 2) {
      anamorphic.x /= ratio;
    }
  } else {
    float edge = smoothstepf(max(0.0f, p.edgeCompressionStart), 1.0f, abs(spherical.x));
    float edgeCompression = (p.edgeCompression * p.edgeCompressionScale + axisWarp * axisDelta * 0.08f) * edge *
                            edge * (1.0f - abs(spherical.y) * 0.25f);
    anamorphic.x -= (anamorphic.x < 0.0f ? -1.0f : 1.0f) * edgeCompression;
  }
  float fovScale = 1.0f + p.horizontalFovBoost * (70.0f / max(10.0f, p.virtualFocalLength));
  anamorphic.x /= max(0.05f, fovScale);
  float mumpsMask = exp(-(radius * radius) / 0.22f);
  float mumps = (p.closeFocusMumps - p.faceWidthCompensation) * smoothstepf(1.0f, 0.0f, p.focusDistance) * p.mumpsScale *
                mumpsMask;
  anamorphic.x /= max(0.1f, 1.0f + mumps * (1.0f - clamp01(p.centerProtection) * 0.35f));

  BasicLensMap m;
  m.sampleCoord = float2(lerpf(spherical.x, anamorphic.x, transferAmount), lerpf(spherical.y, anamorphic.y, transferAmount));
  m.sampleCoord = finiteOr2(m.sampleCoord, spherical);
  return m;
}

float2 applyEdgeCrop(float dstX, float dstY, constant I &info, constant P &p) {
  float scale = max(1.0f, p.edgeCropScale);
  if (scale <= 1.0001f) {
    return float2(dstX, dstY);
  }
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  return float2(cx + (dstX - cx) / scale, cy + (dstY - cy) / scale);
}

float4 warpedSourceSample(const device PixelChannel *src, constant I &info, constant P &p, float dstX, float dstY,
                          float caPixels) {
  float2 cropped = applyEdgeCrop(dstX, dstY, info, p);
  LensMap lm = buildLensMap(cropped.x, cropped.y, info, p);
  float2 sp = lensMapToSourcePixel(lm, info);
  float caMask = p.edgeOnlyCA > 0.5f ? smoothstepf(0.2f, 1.0f, lm.radius) : 1.0f;
  sp += lm.caDirection * caPixels * caMask;
  return sampleBilinear(src, info, sp.x, sp.y);
}

float4 warpedBokehSourceSample(const device PixelChannel *src,
                               constant I &info,
                               constant P &p,
                               float dstX,
                               float dstY,
                               float caPixels) {
  return sampleBilinearZero(src, info, dstX, dstY);
}

float qualityScale(constant P &p) {
  return p.renderQuality == 0 ? 0.35f : (p.renderQuality == 1 ? 0.65f : (p.renderQuality == 3 ? 1.5f : 1.0f));
}

struct CoCInfo {
  float total;
  float nearValue;
  float farValue;
  float focusMask;
};

float normaliseDepthValue(float d, constant P &p) {
  d = p.invertDepth != 0 ? 1.0f - d : d;
  d = (d - p.depthBlackPoint) / max(p.depthWhitePoint - p.depthBlackPoint, 0.00001f);
  d = clamp01(d);
  return pow(d, max(p.depthGamma, 0.00001f));
}

float normalisedDepthAt(const device PixelChannel *depth, constant I &info, constant P &p, float x, float y) {
  if (info.hasDepth == 0 || p.enableDepthMap == 0) {
    return clamp01(p.focusDistance);
  }
  return normaliseDepthValue(sampleDepthBilinear(depth, info, x, y), p);
}

float smoothedDepthAt(const device PixelChannel *depth, constant I &info, constant P &p, float x, float y) {
  float centre = normalisedDepthAt(depth, info, p, x, y);
  int radius = min(max(p.depthSmoothRadius, 0), 2);
  if (radius == 0 || info.hasDepth == 0 || p.enableDepthMap == 0) {
    return centre;
  }

  float sum = 0.0f;
  float weightSum = 0.0f;
  float spatialSigma = max(1.0f, float(radius * radius));
  float rangeStrength = max(0.0f, p.depthEdgeProtect);
  for (int oy = -2; oy <= 2; ++oy) {
    for (int ox = -2; ox <= 2; ++ox) {
      if (abs(ox) > radius || abs(oy) > radius) {
        continue;
      }
      float2 o = float2(float(ox), float(oy));
      float d = normalisedDepthAt(depth, info, p, x + o.x, y + o.y);
      float spatial = exp(-dot(o, o) / max(spatialSigma, 0.00001f));
      float range = exp(-abs(d - centre) * rangeStrength);
      float w = spatial * range;
      sum += d * w;
      weightSum += w;
    }
  }
  return weightSum > 0.0f ? sum / weightSum : centre;
}

CoCInfo computeCoC(float depthValue, constant P &p) {
  CoCInfo c;
  c.nearValue = 0.0f;
  c.farValue = 8.0f;
  c.total = 8.0f;
  c.focusMask = 1.0f;
  return c;
}

float highlightMask(float4 color, constant P &p) {
  float luma = luminance(color);
  return smoothstepf(p.highlightThreshold, p.highlightThreshold + p.highlightSoftness, luma);
}

float2 rotate2D(float2 v, float radians) {
  float s = sin(radians);
  float c = cos(radians);
  return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

int cappedBokehSamples(constant P &p) {
  return p.renderQuality == 0 ? 24 : (p.renderQuality == 1 ? 40 : (p.renderQuality == 3 ? 96 : 64));
}

float hash12(float2 p) {
  float h = sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453123f;
  return fract(h);
}

float2 apertureSampleVogel(int i, int n, float jitterAngle) {
  float fi = float(i) + 0.5f;
  float r = sqrt(fi / max(1.0f, float(n)));
  float theta = fi * 2.39996323f + jitterAngle;
  return float2(cos(theta), sin(theta)) * r;
}

float apertureProfile(float r2, constant P &p) {
  if (r2 > 1.0f) {
    return 0.0f;
  }
  float r = sqrt(max(0.0f, r2));
  float softStart = max(0.0f, 1.0f - p.apertureSoftness);
  float softEdge = 1.0f - smoothstepf(softStart, 1.0f, r);
  float rim = smoothstepf(0.72f, 1.0f, r);
  float centre = 1.0f - smoothstepf(0.0f, 1.0f, r);
  return max(0.0f, (1.0f + rim * p.rimBrightness + centre * p.centreDensity) * softEdge);
}

float bokehOcclusionWeight(float centreDepth, float sampleDepth, constant P &p) {
  float diff = sampleDepth - centreDepth;
  if (diff > p.occlusionThreshold) {
    return exp(-diff * p.foregroundEdgeProtect);
  }
  if (diff < -p.occlusionThreshold) {
    return exp(diff * p.backgroundEdgeProtect);
  }
  return exp(-abs(diff) * p.depthEdgeProtect * 0.15f);
}

float4 bokehSourceSample(const device PixelChannel *src,
                         constant I &info,
                         constant P &p,
                         float x,
                         float y,
                         float2 offsetPx,
                         bool allowCA) {
  if (!allowCA || p.bokehCAEnable == 0 || p.bokehCAAmount <= 0.0001f) {
    return warpedBokehSourceSample(src, info, p, x + offsetPx.x, y + offsetPx.y, 0.0f);
  }

  float len = length(offsetPx);
  float2 dir = len > 0.0001f ? offsetPx / len : float2(1.0f, 0.0f);
  float caPixels = p.bokehCAAmount * 4.0f;
  float4 centre = warpedBokehSourceSample(src, info, p, x + offsetPx.x, y + offsetPx.y, 0.0f);
  float4 red = warpedBokehSourceSample(src,
                                       info,
                                       p,
                                       x + offsetPx.x + dir.x * caPixels,
                                       y + offsetPx.y + dir.y * caPixels,
                                       0.0f);
  float4 blue = warpedBokehSourceSample(src,
                                        info,
                                        p,
                                        x + offsetPx.x - dir.x * caPixels,
                                        y + offsetPx.y - dir.y * caPixels,
                                        0.0f);
  return float4(red.x, centre.y, blue.z, centre.w);
}

float3 saturateBokeh(float3 rgb, float amount) {
  float l = dot(rgb, float3(0.2126f, 0.7152f, 0.0722f));
  return mix(float3(l), rgb, amount);
}

float3 softClipBokeh(float3 x, float rolloff) {
  return x / max(float3(0.00001f), float3(1.0f) + x * rolloff);
}

float4 depthAwareOvalGather(const device PixelChannel *src,
                            const device PixelChannel *depth,
                            constant I &info,
                            constant P &p,
                            float x,
                            float y,
                            float radius,
                            float centreDepth,
                            bool highlightOnly) {
  (void)depth;
  (void)centreDepth;

  if (radius < 0.35f) {
    float4 s = sampleBilinearZero(src, info, x, y);
    if (highlightOnly) {
      float h = highlightMask(s, p);
      return float4(saturateBokeh(s.xyz * h, p.highlightSaturation), s.w);
    }
    return s;
  }

  float4 acc = float4(0.0f);
  float weightSum = 0.0f;
  int tapCount = cappedBokehSamples(p);
  float jitterAngle = hash12(float2(x, y)) * (2.0f * kPi);
  float oval = max(1.0f, p.ovalRatio);
  float customAngle = p.ovalAngle * (kPi / 180.0f);

  for (int i = 0; i < tapCount; ++i) {
    float2 a = apertureSampleVogel(i, tapCount, jitterAngle);
    float2 aperture = a;
    if (p.ovalOrientation == 1) {
      aperture.x *= oval;
    } else {
      aperture.y *= oval;
      if (p.ovalOrientation == 2) {
        aperture = rotate2D(aperture, customAngle);
      }
    }

    float2 offsetPx = aperture * radius;
    float4 tap = sampleBilinearZero(src, info, x + offsetPx.x, y + offsetPx.y);
    if (highlightOnly) {
      float h = highlightMask(tap, p);
      tap = float4(saturateBokeh(tap.xyz * h, p.highlightSaturation), tap.w);
    }
    float w = 1.0f;
    acc += tap * w;
    weightSum += w;
  }

  return acc / max(weightSum, 1e-6f);
}

int cappedEdgeBlurSamples(constant P &p) {
  return p.renderQuality == 0 ? 1 : (p.renderQuality == 1 ? 2 : (p.renderQuality == 3 ? 6 : 4));
}

int cappedSmearSamples(constant P &p) {
  return p.renderQuality == 0 ? 1 : (p.renderQuality == 1 ? 2 : (p.renderQuality == 3 ? 5 : 3));
}

int cappedFlareSteps(constant P &p) {
  int requested = max(1, int(round((2.0f + p.flareLength * p.flareStepDensity) * qualityScale(p))));
  int cap = p.renderQuality == 0 ? 8 : (p.renderQuality == 1 ? 16 : (p.renderQuality == 3 ? 48 : 32));
  return min(requested, cap);
}

int cappedBloomRings(constant P &p) {
  int requested = max(1, int(round(float(p.bloomRings) * qualityScale(p))));
  int cap = p.renderQuality == 0 ? 1 : (p.renderQuality == 1 ? 2 : (p.renderQuality == 3 ? 8 : 3));
  return min(requested, cap);
}

int cappedBloomSamplesPerRing(constant P &p) {
  int requested = max(3, int(round(float(p.bloomSamplesPerRing) * qualityScale(p))));
  int cap = p.renderQuality == 0 ? 4 : (p.renderQuality == 1 ? 6 : (p.renderQuality == 3 ? 16 : 8));
  return min(requested, cap);
}

int cappedGhostCount(constant P &p) {
  int cap = p.renderQuality == 0 ? 0 : (p.renderQuality == 1 ? 1 : (p.renderQuality == 3 ? 8 : 4));
  return min(p.ghostCount, cap);
}

float cappedCAPixels(constant P &p) {
  float cap = p.renderQuality == 0 ? 1.0f : (p.renderQuality == 1 ? 2.0f : (p.renderQuality == 3 ? 8.0f : 4.0f));
  return min(p.lateralCA * p.lateralCAPixelScale, cap);
}

float4 opticalBaseSample(const device PixelChannel *src, constant I &info, constant P &p, float x, float y, LocalMapping map, const device PixelChannel *depth) {
  float caPixels = p.lateralCA * p.lateralCAPixelScale;
  float4 base = fastSourceSampleCA(src, info, p, map, 0.0f, 0.0f, 0.0f);
  if (caPixels > 0.01f) {
    float cappedCA = cappedCAPixels(p);
    base.x = fastSourceSampleCA(src, info, p, map, 0.0f, 0.0f, cappedCA).x;
    base.z = fastSourceSampleCA(src, info, p, map, 0.0f, 0.0f, -cappedCA).z;
  }
  if (p.longitudinalCA > 0.001f) {
    float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
    float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
    float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f);
    float ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
    float edge = smoothstepf(0.15f, 1.05f, sqrt(nx * nx + ny * ny));
    float depthValue = sampleDepthBilinear(depth, info, x, y);
    float focusBias = 0.35f + abs(p.focusDistance - depthValue) * 1.3f;
    float radiusPixels = p.longitudinalCA * focusBias * edge * 4.0f;
    float amount = clamp01(p.longitudinalCA * (0.35f + edge * 0.65f));

    float2 d = float2(x - cx, y - cy);
    float len = length(d);
    d = len > 0.0001f ? d / len : float2(1.0f, 0.0f);

    if (radiusPixels > 0.05f) {
      float cappedCA = caPixels > 0.01f ? cappedCAPixels(p) : 0.0f;
      float4 center = fastSourceSampleCA(src, info, p, map, 0.0f, 0.0f, 0.0f);
      float4 outward = fastSourceSampleCA(src, info, p, map, d.x * radiusPixels, d.y * radiusPixels, cappedCA);
      float4 inward = fastSourceSampleCA(src, info, p, map, -d.x * radiusPixels, -d.y * radiusPixels, cappedCA);
      float defX = center.x * 0.5f + (outward.x + inward.x) * 0.25f;

      float4 outwardB = fastSourceSampleCA(src, info, p, map, d.x * radiusPixels * 0.65f, d.y * radiusPixels * 0.65f, -cappedCA);
      float4 inwardB = fastSourceSampleCA(src, info, p, map, -d.x * radiusPixels * 0.65f, -d.y * radiusPixels * 0.65f, -cappedCA);
      float defZ = center.z * 0.5f + (outwardB.z + inwardB.z) * 0.25f;

      base.x = lerpf(base.x, defX, amount);
      base.z = lerpf(base.z, defZ, amount);
    }
  }
  return base;
}

float4 edgeCharacter(const device PixelChannel *src, constant I &info, constant P &p, float x, float y, float4 base, LocalMapping map, LensMap lm, const device PixelChannel *depth) {
  if (p.edgeBlur <= 0.001f && p.fieldCurvature <= 0.001f && p.tangentialSmear <= 0.001f &&
      p.horizontalSmear <= 0.001f && p.verticalSharpness <= 0.001f) {
    return base;
  }

  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f);
  float ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
  float radius = max(lm.radius, sqrt(nx * nx + ny * ny));
  float edge = max(lm.edgeMask, smoothstepf(max(0.0f, 1.0f - p.radialFalloff), 1.15f, radius));
  float4 result = base;
  float depthValue = sampleDepthBilinear(depth, info, x, y);
  float focusBias = 0.35f + abs(p.focusDistance - depthValue) * 1.3f;
  float blurRadius = p.edgeBlur * edge * focusBias * p.edgeBlurPixels + p.fieldCurvature * edge * p.fieldCurvaturePixels;
  if (blurRadius > 0.05f) {
    float4 blur = 0.0f;
    float weight = 0.0f;
    int blurSamples = cappedEdgeBlurSamples(p);
    for (int i = -blurSamples; i <= blurSamples; ++i) {
      float t = float(i) / float(blurSamples);
      float w = 1.0f - abs(t) * 0.55f;
      blur += fastSourceSampleCA(src, info, p, map, t * blurRadius, t * blurRadius * 0.25f, 0.0f) * w;
      weight += w;
    }
    result = lerp4(result, blur / weight, clamp01(edge * (p.edgeBlur + p.fieldCurvature)));
  }
  float smearRadius = edge * (p.tangentialSmear + p.horizontalSmear) * p.smearPixels;
  if (smearRadius > 0.05f) {
    float4 smear = 0.0f;
    float weight = 0.0f;
    int smearSamples = cappedSmearSamples(p);
    for (int i = -smearSamples; i <= smearSamples; ++i) {
      float t = float(i) / float(smearSamples);
      float w = 1.0f - abs(t) * 0.7f;
      smear += fastSourceSampleCA(src, info, p, map, t * smearRadius, 0.0f, 0.0f) * w;
      weight += w;
    }
    result = lerp4(result, smear / weight, clamp01(edge * (p.tangentialSmear + p.horizontalSmear)));
  }
  if (p.verticalSharpness > 0.001f) {
    float4 up = fastSourceSampleCA(src, info, p, map, 0.0f, -1.5f, 0.0f);
    float4 down = fastSourceSampleCA(src, info, p, map, 0.0f, 1.5f, 0.0f);
    float sharpen = p.verticalSharpness * (1.0f - edge * 0.5f);
    result.y = clamp01(result.y + (result.y - (up.y + down.y) * 0.5f) * sharpen);
    result.x = clamp01(result.x + (result.x - (up.x + down.x) * 0.5f) * sharpen * 0.5f);
    result.z = clamp01(result.z + (result.z - (up.z + down.z) * 0.5f) * sharpen * 0.5f);
  }
  return result;
}

float4 additionalBackgroundBlur(const device PixelChannel *src,
                               constant I &info,
                               constant P &p,
                               float x,
                               float y,
                               float4 base,
                               LocalMapping map,
                               const device PixelChannel *depth) {
  if (p.enableAdditionalBackgroundBlur == 0) {
    return base;
  }

  float focusDelta = 0.0f;
  if (info.hasDepth != 0) {
    float depthValue = sampleDepthBilinear(depth, info, x, y);
    focusDelta = abs(p.focusDistance - depthValue);
  }

  float luma = luminance(base);
  float backgroundMask = info.hasDepth != 0 ? clamp01((focusDelta - 0.04f) * 1.8f)
                                            : clamp01((0.7f - luma) * 1.2f);
  float highlightSuppress = 1.0f - smoothstepf(0.55f, 0.95f, luma);
  float amount = backgroundMask * highlightSuppress;
  if (amount <= 0.001f) {
    return base;
  }

  float radius = (6.0f + p.bloomRadius * p.bloomPixelScale * 0.08f) * (0.35f + backgroundMask * 0.65f);
  if (radius <= 0.35f) {
    return base;
  }

  float2 offsets[8] = {
      float2(1.0f, 0.0f),
      float2(-1.0f, 0.0f),
      float2(0.0f, 1.0f),
      float2(0.0f, -1.0f),
      float2(0.7f, 0.7f),
      float2(-0.7f, 0.7f),
      float2(0.7f, -0.7f),
      float2(-0.7f, -0.7f),
  };

  float4 blur = 0.0f;
  float weight = 0.0f;
  for (int i = 0; i < 8; ++i) {
    float2 o = offsets[i];
    float w = (abs(o.x) > 0.5f && abs(o.y) > 0.5f) ? 0.85f : 1.0f;
    blur += fastSourceSampleCA(src, info, p, map, o.x * radius, o.y * radius, 0.0f) * w;
    weight += w;
  }

  if (weight <= 0.0f) {
    return base;
  }

  blur /= weight;
  blur.w = base.w;
  float blend = min(0.55f, amount * 0.5f + p.edgeBlur * 0.15f);
  return lerp4(base, blur, blend);
}

float edgeMaskAt(constant I &info, constant P &p, float x, float y) {
  float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
  float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
  float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f);
  float ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
  LensMap lm = buildLensMap(x, y, info, p);
  float radius = max(lm.radius, sqrt(nx * nx + ny * ny));
  return clamp01(max(lm.edgeMask, smoothstepf(max(0.0f, 1.0f - p.radialFalloff), 1.15f, radius)));
}

float highlightAt(const device PixelChannel *src, constant I &info, constant P &p, float x, float y, float threshold) {
  return smoothstepf(threshold, 1.0f, luminance(warpedSourceSample(src, info, p, x, y, 0.0f)));
}

float4 lensAdditives(const device PixelChannel *src, constant I &info, constant P &p, float x, float y, float4 base, LensMap lm, const device PixelChannel *depth) {
  float4 add = 0.0f;
  if (p.enableHighlightEffects == 0) {
    return add;
  }
  bool flareEnabled = p.flareIntensity > 0.001f && p.flareLength > 0.001f;
  bool bloomEnabled = (p.veil > 0.001f || p.highlightCream > 0.001f) && p.bloomRadius > 0.001f &&
                      p.bloomPixelScale > 0.001f;
  bool ghostEnabled = p.ghostIntensity > 0.001f && p.ghostCount > 0 && p.ghostSpread > 0.001f;
  bool centerVeilEnabled = p.veil > 0.001f && p.centerVeilScale > 0.001f;
  if (!flareEnabled && !bloomEnabled && !ghostEnabled && !centerVeilEnabled) {
    return add;
  }

  float depthValue = sampleDepthBilinear(depth, info, x, y);
  float focusBias = 0.35f + abs(p.focusDistance - depthValue) * 1.3f;

  int flareSteps = cappedFlareSteps(p);
  float flareSpan = p.flareLength * float(info.width) * p.flareSpanScale * lensIdentityFlareScale(p);
  if (flareEnabled && flareSpan > 1.0f) {
    float dirX = p.flareDirX;
    float dirY = p.flareDirY;
    for (int i = -flareSteps; i <= flareSteps; ++i) {
      if (i == 0) {
        continue;
      }
      float t = float(i) / float(flareSteps);
      float h = highlightAt(src, info, p, x + dirX * t * flareSpan, y + dirY * t * flareSpan, p.flareThreshold);
      float w = exp(-abs(t) * p.flareFalloff) * h * p.flareIntensity;
      add.x += p.flareColourR * w;
      add.y += p.flareColourG * w;
      add.z += p.flareColourB * w;
    }
  }

  float bloomPixels = p.bloomRadius * focusBias * p.bloomPixelScale;
  if (bloomEnabled && bloomPixels > 0.5f) {
    float cosR = p.bokehCos;
    float sinR = p.bokehSin;
    float stretch = max(0.05f, (1.0f + p.bokehStretch * p.bokehStretchScale) * lensIdentityBloomScale(p));
    int rings = cappedBloomRings(p);
    int samplesPerRing = cappedBloomSamplesPerRing(p);
    float total = 0.0f;
    float4 bloom = 0.0f;
    for (int ring = 1; ring <= rings; ++ring) {
      float ringRadius = bloomPixels * float(ring) / float(rings);
      for (int i = 0; i < samplesPerRing; ++i) {
        float a = 2.0f * kPi * float(i) / float(samplesPerRing);
        float ox = cos(a) * ringRadius / stretch;
        float oy = sin(a) * ringRadius * stretch;
        float2 offset = float2(ox * cosR - oy * sinR, ox * sinR + oy * cosR);
        float4 s = warpedSourceSample(src, info, p, x + offset.x, y + offset.y, 0.0f);
        float h = smoothstepf(p.flareThreshold * p.bloomThresholdScale, 1.0f, luminance(s));
        float w = h / float(ring);
        bloom += s * w;
        total += w;
      }
    }
    if (total > 0.0f) {
      bloom /= total;
      float edge = max(lm.edgeMask, smoothstepf(0.35f, 1.1f, lm.radius));
      float bokehEdgeKeep = clamp01(1.0f - edge * p.bokehEdgeFalloff * p.bloomEdgeKeepScale);
      float protect = lerpf(1.0f, clamp01(luminance(base) * 2.0f), p.blackLiftProtection);
      float amount = (p.veil * p.bloomVeilScale + p.highlightCream * p.bloomCreamScale) * protect * bokehEdgeKeep;
      add.xyz += bloom.xyz * amount;
    }
  }

  if (ghostEnabled) {
    float cx = float(info.sourceX1 + info.sourceX2 - 1) * 0.5f;
    float cy = float(info.sourceY1 + info.sourceY2 - 1) * 0.5f;
    float tintShift = p.coatingStyle == 0 ? p.coatingWarmResponse : (p.coatingStyle == 2 ? p.coatingCoolResponse : 1.0f);
    int ghostCount = cappedGhostCount(p);
    for (int i = 1; i <= ghostCount; ++i) {
      float scale = 1.0f + p.ghostSpread * float(i);
      float4 g = warpedSourceSample(src,
                                    info,
                                    p,
                                    cx - (x - cx) * scale * lensIdentityGhostScaleX(p),
                                    cy - (y - cy) * scale * lensIdentityGhostScaleY(p),
                                    0.0f);
      float w = smoothstepf(p.flareThreshold, 1.0f, luminance(g)) * (p.ghostIntensity / float(i)) * tintShift;
      add.x += g.x * p.ghostTintR * w;
      add.y += g.y * p.ghostTintG * w;
      add.z += g.z * p.ghostTintB * w;
    }
  }

  if (centerVeilEnabled) {
    float centerGlow = smoothstepf(p.flareThreshold * 0.9f, 1.0f, luminance(base));
    add.xyz += float3(p.veil * centerGlow * p.centerVeilScale);
  }

  return add;
}

float aspectValue(int index, float customOutputAspect) {
  return index == 0 ? 2.0f : (index == 1 ? 2.39f : (index == 2 ? 2.66f : max(0.1f, customOutputAspect)));
}

float4 applyVignetteAndGuides(float4 color, float x, float y, constant I &info, constant P &p) {
  float cx = (float(info.width) - 1.0f) * 0.5f;
  float cy = (float(info.height) - 1.0f) * 0.5f;
  float nx = (x - cx) / max(1.0f, float(info.width) * 0.5f);
  float ny = (y - cy) / max(1.0f, float(info.height) * 0.5f);
  float ovalY = ny * (1.0f + p.ovalVignette * p.ovalVignetteScale);
  float asym = nx * p.vignetteAsymmetry * p.vignetteAsymmetryScale + ny * p.cornerBias * p.vignetteAsymmetryScale;
  float vignette = 1.0f - p.ovalVignette * smoothstepf(0.35f, 1.2f, sqrt(nx * nx + ovalY * ovalY) + asym);
  vignette = max(0.0f, vignette);
  float edge = smoothstepf(0.55f, 1.08f, sqrt(nx * nx + ny * ny));
  float edgeDim = 1.0f - p.catEyeStrength * edge * p.catEyeDimScale - p.bokehVignette * edge * p.bokehVignetteDimScale;
  edgeDim = max(0.0f, edgeDim);
  color.xyz *= vignette * edgeDim;
  if (p.guidesEnabled != 0 || p.letterboxPreview != 0) {
    float target = aspectValue(p.outputAspect, p.customOutputAspect);
    float current = float(info.width) / max(1.0f, float(info.height));
    float contentX1 = 0.0f;
    float contentY1 = 0.0f;
    float contentX2 = float(info.width);
    float contentY2 = float(info.height);
    if (target > current) {
      float ch = float(info.width) / target;
      contentY1 = (float(info.height) - ch) * 0.5f;
      contentY2 = contentY1 + ch;
    } else {
      float cw = float(info.height) * target;
      contentX1 = (float(info.width) - cw) * 0.5f;
      contentX2 = contentX1 + cw;
    }
    bool outside = x < contentX1 || x >= contentX2 || y < contentY1 || y >= contentY2;
    if (p.letterboxPreview != 0 && outside) {
      color.xyz *= 1.0f - clamp01(p.letterboxOpacity);
    }
    if (p.guidesEnabled != 0) {
      float sx1 = contentX1 + (contentX2 - contentX1) * (1.0f - p.safeArea) * 0.5f;
      float sx2 = contentX2 - (contentX2 - contentX1) * (1.0f - p.safeArea) * 0.5f;
      float sy1 = contentY1 + (contentY2 - contentY1) * (1.0f - p.safeArea) * 0.5f;
      float sy2 = contentY2 - (contentY2 - contentY1) * (1.0f - p.safeArea) * 0.5f;
      bool aspectLine = abs(x - contentX1) < 1.0f || abs(x - contentX2) < 1.0f || abs(y - contentY1) < 1.0f ||
                        abs(y - contentY2) < 1.0f;
      bool safeLine = abs(x - sx1) < 1.0f || abs(x - sx2) < 1.0f || abs(y - sy1) < 1.0f || abs(y - sy2) < 1.0f;
      if (aspectLine || safeLine) {
        float strength = aspectLine ? p.guideAspectStrength : p.guideSafeStrength;
        color.x = lerpf(color.x, 1.0f, strength);
        color.y = lerpf(color.y, safeLine ? 0.85f : 0.65f, strength);
        color.z = lerpf(color.z, 0.25f, strength);
      }
    }
  }
  return color;
}

kernel void RimellAnamorphicFloat(const device PixelChannel *src [[buffer(0)]],
                                  device PixelChannel *dst [[buffer(1)]],
                                  constant P &p [[buffer(2)]],
                                  constant I &info [[buffer(3)]],
                                  const device PixelChannel *depth [[buffer(4)]],
                                  uint2 gid [[thread_position_in_grid]]) {
  int x = info.renderX1 + int(gid.x);
  int y = info.renderY1 + int(gid.y);
  if (x >= info.renderX2 || y >= info.renderY2) {
    return;
  }
  int outIndex = (y - info.outputY1) * info.outputRowFloats + (x - info.outputX1) * 4;

  if (p.uniformVersion != kUniformVersion) {
    writePixel(dst, outIndex, float4(1.0f, 0.0f, 1.0f, 1.0f), float4(1.0f, 0.0f, 1.0f, 1.0f));
    return;
  }

  float4 original = sampleNearest(src, info, float(x), float(y));

  if (p.debugView != 0 && p.debugView < 14) {
    float4 debugColor = original;
    if (p.debugView == 1) {
      debugColor = original;
    } else if (p.debugView == 2) {
      float h = highlightAt(src, info, p, float(x), float(y), p.flareThreshold);
      debugColor = float4(h, h, h, 1.0f);
    } else if (p.debugView == 3) {
      float e = edgeMaskAt(info, p, float(x), float(y));
      debugColor = float4(e, e, e, 1.0f);
    } else if (p.debugView == 7) {
      float rawDepth = sampleDepthBilinear(depth, info, float(x), float(y));
      debugColor = float4(rawDepth, rawDepth, rawDepth, 1.0f);
    } else if (p.debugView == 8) {
      float depthValue = smoothedDepthAt(depth, info, p, float(x), float(y));
      debugColor = float4(depthValue, depthValue, depthValue, 1.0f);
    } else if (p.debugView >= 9 && p.debugView <= 13) {
      float depthValue = smoothedDepthAt(depth, info, p, float(x), float(y));
      CoCInfo coc = computeCoC(depthValue, p);
      float maxRadius = max(p.maxBokehRadius, 0.00001f);
      if (p.debugView == 9) {
        float v = clamp01(coc.total / maxRadius);
        debugColor = float4(v, v, v, 1.0f);
      } else if (p.debugView == 10) {
        float v = clamp01(coc.nearValue / maxRadius);
        debugColor = float4(v, v, v, 1.0f);
      } else if (p.debugView == 11) {
        float v = clamp01(coc.farValue / maxRadius);
        debugColor = float4(v, v, v, 1.0f);
      } else if (p.debugView == 12) {
        float v = 1.0f - coc.focusMask;
        debugColor = float4(v, v, v, 1.0f);
      } else {
        float h = highlightMask(original, p);
        debugColor = float4(h, h, h, 1.0f);
      }
    }
    writePixel(dst, outIndex, debugColor, original);
    return;
  }

  if (p.mix <= 0.0001f) {
    writePixel(dst, outIndex, original, original);
    return;
  }

  float2 centerCropped = applyEdgeCrop(float(x), float(y), info, p);
  LensMap centerLm = buildLensMap(centerCropped.x, centerCropped.y, info, p);
  float2 spCenter = lensMapToSourcePixel(centerLm, info);

  float caMask = p.edgeOnlyCA > 0.5f ? smoothstepf(0.2f, 1.0f, centerLm.radius) : 1.0f;
  float2 caDirection = centerLm.caDirection * caMask;

  float2 dxCropped = applyEdgeCrop(float(x) + 1.0f, float(y), info, p);
  LensMap dxLm = buildLensMap(dxCropped.x, dxCropped.y, info, p);
  float2 spDx = lensMapToSourcePixel(dxLm, info) - spCenter;

  float2 dyCropped = applyEdgeCrop(float(x), float(y) + 1.0f, info, p);
  LensMap dyLm = buildLensMap(dyCropped.x, dyCropped.y, info, p);
  float2 spDy = lensMapToSourcePixel(dyLm, info) - spCenter;

  LocalMapping map = { spCenter, spDx, spDy, caDirection };

  float4 color = opticalBaseSample(src, info, p, float(x), float(y), map, depth);

  // DEBUG_DEPTH
  // float debug_depthValue = smoothedDepthAt(depth, info, p, float(x), float(y));
  // return float4(debug_depthValue, debug_depthValue, debug_depthValue, 1.0f);

  // DEBUG_COC
  // CoCInfo debug_c = computeCoC(debug_depthValue, p);
  // float debug_v = debug_c.total / max(p.maxBokehRadius, 0.0001f);
  // return float4(debug_v, debug_v, debug_v, 1.0f);

  if (p.enableEdgeEffects != 0) {
    color = edgeCharacter(src, info, p, float(x), float(y), color, map, centerLm, depth);
  }
  color = additionalBackgroundBlur(src, info, p, float(x), float(y), color, map, depth);
  float3 bokehDebugLayer = 0.0f;
  if (p.enableBokeh != 0 && p.enableDepthMap != 0 && info.hasDepth != 0 && p.maxBokehRadius > 0.001f) {
    float centreDepth = smoothedDepthAt(depth, info, p, float(x), float(y));
    CoCInfo coc = computeCoC(centreDepth, p);
    float baseMask = coc.focusMask * p.bokehAmount;
    if (baseMask > 0.0001f && coc.total > 0.35f) {
      float4 baseBlur = depthAwareOvalGather(src,
                                             depth,
                                             info,
                                             p,
                                             float(x),
                                             float(y),
                                             coc.total,
                                             centreDepth,
                                             false);
      bokehDebugLayer += abs(baseBlur.xyz - color.xyz) * baseMask;
      color.xyz = mix(color.xyz, baseBlur.xyz, baseMask);
      color.w = original.w;
    }

    if (p.highlightBokehEnable != 0 && p.highlightGain > 0.0001f) {
      float highlightRadius = coc.total * p.highlightRadiusMultiplier;
      if (highlightRadius > 0.35f) {
        float4 highlightBokeh = depthAwareOvalGather(src,
                                                     depth,
                                                     info,
                                                     p,
                                                     float(x),
                                                     float(y),
                                                     highlightRadius,
                                                     centreDepth,
                                                     true);
        float3 highlightAdd = softClipBokeh(highlightBokeh.xyz * p.highlightGain, p.highlightRolloff);
        highlightAdd *= p.bokehAmount;
        color.xyz += highlightAdd;
        bokehDebugLayer += highlightAdd;
      }
    }
  }
  if (p.enableHighlightEffects != 0) {
    float4 add = lensAdditives(src, info, p, float(x), float(y), color, centerLm, depth);
    color.xyz += add.xyz;
  }
  color = applyVignetteAndGuides(color, float(x - info.sourceX1), float(y - info.sourceY1), info, p);
  color.w = original.w;

  float blend = clamp01(p.mix);
  float4 outPixel = lerp4(original, color, blend);
  if (p.debugView == 14) {
    outPixel = float4(bokehDebugLayer, original.w);
  } else if (p.debugView == 15) {
    outPixel = float4(abs(outPixel.xyz - original.xyz), original.w);
  }
  outPixel = float4(finiteOr(outPixel.x, original.x),
                    finiteOr(outPixel.y, original.y),
                    finiteOr(outPixel.z, original.z),
                    original.w);
  writePixel(dst, outIndex, outPixel, original);
}

kernel void RimellIdentityFloat(device const float *src [[buffer(0)]],
                                device float *dst [[buffer(1)]],
                                constant P &p [[buffer(2)]],
                                constant I &info [[buffer(3)]],
                                uint2 gid [[thread_position_in_grid]]) {
  int x = info.renderX1 + int(gid.x);
  int y = info.renderY1 + int(gid.y);
  if (x >= info.renderX2 || y >= info.renderY2) {
    return;
  }

  int sx = clamp(x, info.sourceX1, info.sourceX2 - 1) - info.sourceX1;
  int sy = clamp(y, info.sourceY1, info.sourceY2 - 1) - info.sourceY1;
  int ox = x - info.outputX1;
  int oy = y - info.outputY1;
  int sidx = sy * info.sourceRowFloats + sx * 4;
  int oidx = oy * info.outputRowFloats + ox * 4;

  dst[oidx + 0] = src[sidx + 0];
  dst[oidx + 1] = src[sidx + 1];
  dst[oidx + 2] = src[sidx + 2];
  dst[oidx + 3] = src[sidx + 3];
}

kernel void RimellBilinearFloat(const device PixelChannel *src [[buffer(0)]],
                                device PixelChannel *dst [[buffer(1)]],
                                constant P &p [[buffer(2)]],
                                constant I &info [[buffer(3)]],
                                uint2 gid [[thread_position_in_grid]]) {
  int x = info.renderX1 + int(gid.x);
  int y = info.renderY1 + int(gid.y);
  if (x >= info.renderX2 || y >= info.renderY2) {
    return;
  }

  int outIndex = (y - info.outputY1) * info.outputRowFloats + (x - info.outputX1) * 4;
  float4 original = sampleNearest(src, info, float(x), float(y));
  float4 color = sampleBilinear(src, info, float(x), float(y));
  writePixel(dst, outIndex, color, original);
}

kernel void RimellBasicGeometryFloat(const device PixelChannel *src [[buffer(0)]],
                                     device PixelChannel *dst [[buffer(1)]],
                                     constant P &p [[buffer(2)]],
                                     constant I &info [[buffer(3)]],
                                     uint2 gid [[thread_position_in_grid]]) {
  int x = info.renderX1 + int(gid.x);
  int y = info.renderY1 + int(gid.y);
  if (x >= info.renderX2 || y >= info.renderY2) {
    return;
  }

  int outIndex = (y - info.outputY1) * info.outputRowFloats + (x - info.outputX1) * 4;
  float4 original = sampleNearest(src, info, float(x), float(y));
  float2 cropped = applyEdgeCrop(float(x), float(y), info, p);
  BasicLensMap lm = buildBasicLensMap(cropped.x, cropped.y, info, p);
  float2 sp = sampleCoordToSourcePixel(lm.sampleCoord, info);
  float4 color = sampleBilinear(src, info, sp.x, sp.y);
  writePixel(dst, outIndex, color, original);
}

kernel void rimell_copy_float(device const float *source [[buffer(0)]],
                              device float *output [[buffer(1)]],
                              constant CopyUniforms &u [[buffer(2)]],
                              uint2 gid [[thread_position_in_grid]]) {
  int x = int(gid.x) + u.renderX1;
  int y = int(gid.y) + u.renderY1;

  if (x >= u.renderX2 || y >= u.renderY2) {
    return;
  }

  int sx = clamp(x, u.sourceX1, u.sourceX2 - 1) - u.sourceX1;
  int sy = clamp(y, u.sourceY1, u.sourceY2 - 1) - u.sourceY1;
  int ox = x - u.outputX1;
  int oy = y - u.outputY1;

  int sidx = sy * u.sourceRowFloats + sx * 4;
  int oidx = oy * u.outputRowFloats + ox * 4;

  output[oidx + 0] = source[sidx + 0];
  output[oidx + 1] = source[sidx + 1];
  output[oidx + 2] = source[sidx + 2];
  output[oidx + 3] = source[sidx + 3];
}
