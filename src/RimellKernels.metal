#include <metal_stdlib>
using namespace metal;

struct Uniforms {
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

struct DepthState {
  float raw;
  float normalised;
  float distanceFromFocus;
  float defocusMask;
  float subjectProtection;
  float defocusPixels;
};

constant float kPi = 3.14159265358979323846f;

float clamp01(float v) {
  return clamp(v, 0.0f, 1.0f);
}

float luminance(float4 p) {
  return 0.2126f * p.x + 0.7152f * p.y + 0.0722f * p.z;
}

float4 readPixelClamped(device const float *base,
                        int rowFloats,
                        int x,
                        int y,
                        int x1,
                        int y1,
                        int x2,
                        int y2) {
  x = clamp(x, x1, x2 - 1);
  y = clamp(y, y1, y2 - 1);
  int lx = x - x1;
  int ly = y - y1;
  int idx = ly * rowFloats + lx * 4;
  return float4(base[idx], base[idx + 1], base[idx + 2], base[idx + 3]);
}

float4 sampleBilinearClamped(device const float *base,
                             int rowFloats,
                             float x,
                             float y,
                             int x1,
                             int y1,
                             int x2,
                             int y2) {
  int ix = int(floor(x));
  int iy = int(floor(y));
  float tx = x - float(ix);
  float ty = y - float(iy);
  float4 p00 = readPixelClamped(base, rowFloats, ix, iy, x1, y1, x2, y2);
  float4 p10 = readPixelClamped(base, rowFloats, ix + 1, iy, x1, y1, x2, y2);
  float4 p01 = readPixelClamped(base, rowFloats, ix, iy + 1, x1, y1, x2, y2);
  float4 p11 = readPixelClamped(base, rowFloats, ix + 1, iy + 1, x1, y1, x2, y2);
  return mix(mix(p00, p10, tx), mix(p01, p11, tx), ty);
}

float2 sourceCentre(constant Uniforms &u) {
  return float2(float(u.sourceX1 + u.sourceX2 - 1) * 0.5f,
                float(u.sourceY1 + u.sourceY2 - 1) * 0.5f);
}

float edgeMaskForPoint(float x, float y, constant Uniforms &u) {
  float2 c = sourceCentre(u);
  float nx = (x - c.x) / max(1.0f, float(u.width) * 0.5f);
  float ny = (y - c.y) / max(1.0f, float(u.height) * 0.5f);
  float r = sqrt(nx * nx + ny * ny);
  float edgeX = smoothstep(max(0.0f, u.edgeCompressionStart), 1.0f, abs(nx));
  float edgeR = smoothstep(max(0.0f, 1.0f - u.radialFalloff), 1.15f, r);
  return clamp01(max(edgeX, edgeR));
}

float2 virtualAnamorphicMap(float x, float y, constant Uniforms &u) {
  float2 c = sourceCentre(u);
  float halfW = max(1.0f, float(u.width) * 0.5f);
  float halfH = max(1.0f, float(u.height) * 0.5f);
  float2 p = float2((x - c.x) / halfW, (y - c.y) / halfH);
  float aspect = halfW / halfH;
  float radius = length(float2(p.x * aspect, p.y));
  float centreRadius = 0.28f + clamp01(u.centreProtection) * 0.42f;
  float transfer = clamp01(u.anamorphicTransfer) * smoothstep(centreRadius, centreRadius + 0.2f, radius);
  float axisDelta = max(0.0f, u.squeezeRatio - 1.0f);
  float axisWarp = clamp01(u.axisWarp);
  float axisY = p.y / max(0.1f, 1.0f + axisDelta * (0.6f + axisWarp * 0.7f));
  float r2 = p.x * p.x + axisY * axisY;
  float r4 = r2 * r2;
  float scaleX = 1.0f + (u.barrel + axisWarp * axisDelta * 0.18f) * r2 + u.mustache * r4;
  float scaleY = 1.0f + (u.barrel - axisWarp * axisDelta * 0.07f) * r2 + u.mustache * r4;
  float2 mapped = float2(p.x * scaleX, p.y * scaleY);
  mapped.y *= 1.0f - u.verticalCompensation * r2 * 0.35f;
  float edge = smoothstep(max(0.0f, u.edgeCompressionStart), 1.0f, abs(p.x));
  mapped.x -= sign(mapped.x) * u.edgeCompression * 0.16f * edge * edge * (1.0f - abs(p.y) * 0.25f);
  float2 finalP = mix(p, mapped, transfer);
  return float2(c.x + finalP.x * halfW, c.y + finalP.y * halfH);
}

float4 sampleSource(device const float *source, constant Uniforms &u, float x, float y) {
  return sampleBilinearClamped(source, u.sourceRowFloats, x, y,
                              u.sourceX1, u.sourceY1, u.sourceX2, u.sourceY2);
}

float4 sampleMappedSource(device const float *source, constant Uniforms &u, float x, float y, float caPixels) {
  float2 mapped = virtualAnamorphicMap(x, y, u);
  float2 c = sourceCentre(u);
  float2 dir = mapped - float2(x, y);
  float len = length(dir);
  if (len <= 0.00001f) {
    dir = normalize(float2(x - c.x, y - c.y) + float2(0.0001f, 0.0f));
  } else {
    dir /= len;
  }
  return sampleSource(source, u, mapped.x + dir.x * caPixels, mapped.y + dir.y * caPixels);
}

DepthState makeDepthState(device const float *depthBase, constant Uniforms &u, int x, int y) {
  DepthState d;
  d.raw = 0.5f;
  d.normalised = 0.5f;
  d.distanceFromFocus = 0.0f;
  d.defocusMask = 0.0f;
  d.subjectProtection = 1.0f;
  d.defocusPixels = 0.0f;
  if (u.depthAvailable == 0 || u.depthRowFloats <= 0 ||
      u.depthX1 >= u.depthX2 || u.depthY1 >= u.depthY2) {
    return d;
  }

  float4 dp = readPixelClamped(depthBase, u.depthRowFloats, x, y,
                               u.depthX1, u.depthY1, u.depthX2, u.depthY2);
  float v = isfinite(dp.x) ? dp.x : 0.5f;
  v = clamp01(v);
  if (u.invertDepth != 0) {
    v = 1.0f - v;
  }
  float dist = abs(v - u.focusDepth);
  float start = max(0.0001f, u.focusRange);
  float end = max(start + 0.0001f, u.focusRange + u.depthFalloff);
  float mask = clamp01(smoothstep(start, end, dist) * u.depthInfluence);
  d.raw = dp.x;
  d.normalised = v;
  d.distanceFromFocus = dist;
  d.defocusMask = mask;
  d.subjectProtection = 1.0f - mask;
  d.defocusPixels = clamp(u.depthDefocusPixels * mask, 0.0f, 64.0f);
  return d;
}

float4 depthDefocus(device const float *source,
                    constant Uniforms &u,
                    float x,
                    float y,
                    float4 base,
                    DepthState depthState) {
  if (depthState.defocusPixels <= 0.05f) {
    return base;
  }
  float rotation = u.bokehRotation * kPi / 180.0f;
  float cr = cos(rotation);
  float sr = sin(rotation);
  float stretch = 1.0f + u.bokehStretch;
  int samples = u.renderQuality == 2 ? 12 : (u.renderQuality == 0 ? 5 : 8);
  float4 blur = float4(0.0f);
  for (int i = 0; i < samples; ++i) {
    float a = 2.0f * kPi * float(i) / float(samples);
    float ox = cos(a) * depthState.defocusPixels / max(0.2f, stretch);
    float oy = sin(a) * depthState.defocusPixels * stretch;
    float2 r = float2(ox * cr - oy * sr, ox * sr + oy * cr);
    blur += sampleMappedSource(source, u, x + r.x, y + r.y, 0.0f);
  }
  blur /= float(samples);
  return mix(base, blur, clamp01(depthState.defocusMask));
}

float4 edgeCharacter(device const float *source,
                     constant Uniforms &u,
                     float x,
                     float y,
                     float4 base,
                     DepthState depthState) {
  float edge = max(edgeMaskForPoint(x, y, u), depthState.defocusMask * 0.7f);
  float blurRadius = clamp((u.edgeBlur * 10.0f + depthState.defocusPixels * 0.5f) * edge, 0.0f, 64.0f);
  float4 result = base;
  if (blurRadius > 0.05f) {
    float4 blur = float4(0.0f);
    float weight = 0.0f;
    for (int i = -3; i <= 3; ++i) {
      float t = float(i) / 3.0f;
      float w = 1.0f - abs(t) * 0.55f;
      blur += sampleMappedSource(source, u, x + t * blurRadius, y + t * blurRadius * 0.25f, 0.0f) * w;
      weight += w;
    }
    result = mix(result, blur / weight, clamp01(edge * u.edgeBlur));
  }

  float smearRadius = clamp(edge * (u.tangentialSmear + u.horizontalSmear) * 18.0f *
                                (0.15f + depthState.defocusMask * 0.85f),
                            0.0f,
                            64.0f);
  if (smearRadius > 0.05f) {
    float4 smear = float4(0.0f);
    float weight = 0.0f;
    for (int i = -4; i <= 4; ++i) {
      float t = float(i) / 4.0f;
      float w = 1.0f - abs(t) * 0.7f;
      smear += sampleMappedSource(source, u, x + t * smearRadius, y, 0.0f) * w;
      weight += w;
    }
    result = mix(result, smear / weight, clamp01(edge * (u.tangentialSmear + u.horizontalSmear)));
  }
  return result;
}

float highlightMatte(float4 sample, constant Uniforms &u, float thresholdScale) {
  return clamp01(smoothstep(u.flareThreshold * thresholdScale, 1.0f, luminance(sample)));
}

float4 highlightAdditives(device const float *source,
                          device const float *depth,
                          constant Uniforms &u,
                          float x,
                          float y,
                          float4 base,
                          DepthState localDepth) {
  float4 add = float4(0.0f);
  float flareAngle = u.flareAngle * kPi / 180.0f;
  float2 dir = float2(cos(flareAngle), sin(flareAngle));
  int flareSteps = clamp(int(12.0f + u.flareLength * 44.0f), 4, 64);
  float flareSpan = u.flareLength * float(u.width) * 0.75f;
  if (u.flareIntensity > 0.001f && flareSpan > 1.0f) {
    for (int i = -flareSteps; i <= flareSteps; ++i) {
      if (i == 0) {
        continue;
      }
      float t = float(i) / float(flareSteps);
      float sx = x + dir.x * t * flareSpan;
      float sy = y + dir.y * t * flareSpan;
      float4 s = sampleMappedSource(source, u, sx, sy, 0.0f);
      DepthState sd = makeDepthState(depth, u, int(sx), int(sy));
      float gate = u.depthAvailable != 0 ? max(0.1f, sd.defocusMask) : 1.0f;
      float w = exp(-abs(t) * 3.0f) * highlightMatte(s, u, 1.0f) * u.flareIntensity * gate;
      add.x += u.flareColourR * w;
      add.y += u.flareColourG * w;
      add.z += u.flareColourB * w;
    }
  }

  if ((u.veil > 0.001f || u.highlightCream > 0.001f) && u.bloomRadius > 0.5f) {
    float rotation = u.bokehRotation * kPi / 180.0f;
    float cr = cos(rotation);
    float sr = sin(rotation);
    float stretch = 1.0f + u.bokehStretch;
    float4 bloom = float4(0.0f);
    float total = 0.0f;
    for (int ring = 1; ring <= 3; ++ring) {
      float ringRadius = u.bloomRadius * float(ring) / 3.0f;
      for (int i = 0; i < 10; ++i) {
        float a = 2.0f * kPi * float(i) / 10.0f;
        float ox = cos(a) * ringRadius / max(0.2f, stretch);
        float oy = sin(a) * ringRadius * stretch;
        float2 r = float2(ox * cr - oy * sr, ox * sr + oy * cr);
        float4 s = sampleMappedSource(source, u, x + r.x, y + r.y, 0.0f);
        float h = highlightMatte(s, u, 0.75f);
        if (u.depthAvailable != 0) {
          DepthState sd = makeDepthState(depth, u, int(x + r.x), int(y + r.y));
          h *= mix(1.0f, max(0.1f, sd.defocusMask), clamp01(u.depthBloomBoost));
        }
        float w = h / float(ring);
        bloom += s * w;
        total += w;
      }
    }
    if (total > 0.0f) {
      bloom /= total;
      float amount = (u.veil * 0.4f + u.highlightCream * 0.8f) *
                     (u.depthAvailable != 0 ? 0.15f + 0.85f * localDepth.defocusMask : 1.0f);
      add.xyz += bloom.xyz * amount;
    }
  }

  float centreGlow = highlightMatte(base, u, 0.9f);
  float veilGate = u.depthAvailable != 0 ? 0.2f + localDepth.defocusMask * 0.8f : 1.0f;
  add.xyz += u.veil * centreGlow * 0.08f * veilGate;
  return add;
}

float4 applyVignette(float4 color, float x, float y, constant Uniforms &u) {
  float cx = (float(u.width) - 1.0f) * 0.5f;
  float cy = (float(u.height) - 1.0f) * 0.5f;
  float nx = (x - cx) / max(1.0f, float(u.width) * 0.5f);
  float ny = (y - cy) / max(1.0f, float(u.height) * 0.5f);
  float ovalY = ny * (1.0f + u.ovalVignette * 1.8f);
  float vignette = 1.0f - u.ovalVignette * smoothstep(0.35f, 1.2f, sqrt(nx * nx + ovalY * ovalY));
  float edge = smoothstep(0.55f, 1.08f, sqrt(nx * nx + ny * ny));
  color.xyz *= vignette;
  color.xyz *= 1.0f - u.catEyeStrength * edge * 0.22f;
  return color;
}

kernel void rimell_anamorphic_main(device const float *source [[buffer(0)]],
                                   device float *output [[buffer(1)]],
                                   device const float *depth [[buffer(2)]],
                                   constant Uniforms &u [[buffer(3)]],
                                   uint2 gid [[thread_position_in_grid]]) {
  int x = int(gid.x) + u.renderX1;
  int y = int(gid.y) + u.renderY1;
  if (x >= u.renderX2 || y >= u.renderY2) {
    return;
  }

  int outIndex = (y - u.outputY1) * u.outputRowFloats + (x - u.outputX1) * 4;
  float4 original = readPixelClamped(source, u.sourceRowFloats, x, y,
                                     u.sourceX1, u.sourceY1, u.sourceX2, u.sourceY2);
  DepthState depthState = makeDepthState(depth, u, x, y);

  if (u.debugView == 1) {
    output[outIndex] = original.x;
    output[outIndex + 1] = original.y;
    output[outIndex + 2] = original.z;
    output[outIndex + 3] = original.w;
    return;
  }
  if (u.debugView == 2) {
    float2 mapped = virtualAnamorphicMap(float(x), float(y), u);
    float uu = (mapped.x - float(u.sourceX1)) / max(1.0f, float(u.width - 1));
    float vv = (mapped.y - float(u.sourceY1)) / max(1.0f, float(u.height - 1));
    original = float4(clamp01(uu), clamp01(vv), edgeMaskForPoint(float(x), float(y), u), original.w);
    output[outIndex] = original.x;
    output[outIndex + 1] = original.y;
    output[outIndex + 2] = original.z;
    output[outIndex + 3] = original.w;
    return;
  }
  if (u.debugView == 3 || u.debugView == 4 || u.debugView == 5 || u.debugView == 6 ||
      u.debugView == 7 || u.debugView == 8 || u.debugView == 9) {
    float v = 0.0f;
    if (u.debugView == 3) {
      v = depthState.normalised;
    } else if (u.debugView == 4) {
      v = 1.0f - depthState.defocusMask;
    } else if (u.debugView == 5) {
      v = clamp01(depthState.defocusPixels / 64.0f);
    } else if (u.debugView == 6) {
      v = edgeMaskForPoint(float(x), float(y), u);
    } else if (u.debugView == 7 || u.debugView == 8) {
      v = highlightMatte(sampleMappedSource(source, u, float(x), float(y), 0.0f), u, 1.0f);
    } else if (u.debugView == 9) {
      v = highlightMatte(sampleMappedSource(source, u, float(x), float(y), 0.0f), u, 0.75f);
    }
    output[outIndex] = v;
    output[outIndex + 1] = v;
    output[outIndex + 2] = v;
    output[outIndex + 3] = original.w;
    return;
  }

  if (u.mix <= 0.0001f) {
    output[outIndex] = original.x;
    output[outIndex + 1] = original.y;
    output[outIndex + 2] = original.z;
    output[outIndex + 3] = original.w;
    return;
  }

  float4 base = sampleMappedSource(source, u, float(x), float(y), 0.0f);
  base.x = sampleMappedSource(source, u, float(x), float(y), u.lateralCA).x;
  base.z = sampleMappedSource(source, u, float(x), float(y), -u.lateralCA).z;
  if (u.longitudinalCA > 0.001f) {
    float caRadius = u.longitudinalCA * 4.0f * max(0.2f, depthState.defocusMask);
    base.x = mix(base.x, sampleMappedSource(source, u, float(x + caRadius), float(y), 0.0f).x,
                 clamp01(u.longitudinalCA * depthState.defocusMask));
    base.z = mix(base.z, sampleMappedSource(source, u, float(x - caRadius), float(y), 0.0f).z,
                 clamp01(u.longitudinalCA * depthState.defocusMask));
  }
  base = edgeCharacter(source, u, float(x), float(y), base, depthState);
  base = depthDefocus(source, u, float(x), float(y), base, depthState);
  float4 add = highlightAdditives(source, depth, u, float(x), float(y), base, depthState);

  if (u.debugView == 10) {
    output[outIndex] = add.x;
    output[outIndex + 1] = add.y;
    output[outIndex + 2] = add.z;
    output[outIndex + 3] = original.w;
    return;
  }

  float4 result = base + add;
  result = applyVignette(result, float(x - u.sourceX1), float(y - u.sourceY1), u);
  result = mix(original, result, clamp01(u.mix));
  result.w = original.w;

  output[outIndex] = result.x;
  output[outIndex + 1] = result.y;
  output[outIndex + 2] = result.z;
  output[outIndex + 3] = result.w;
}
