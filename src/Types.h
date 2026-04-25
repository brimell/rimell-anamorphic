#pragma once

#include "ofxCore.h"

namespace rimell {

struct Vec3 {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
};

struct Pixel {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 1.0f;
};

struct Image {
  void *data = nullptr;
  OfxRectI bounds{};
  int rowBytes = 0;
};

struct RenderParams {
  float mix = 1.0f;
  int squeezeMode = 2;
  float squeezeRatio = 1.33f;
  float horizontalFovBoost = 0.0f;
  float virtualFocalLength = 50.0f;
  float breathingScale = 0.12f;

  float bokehStretch = 0.15f;
  float bokehRotation = 0.0f;
  float bokehEdgeFalloff = 0.15f;
  float bokehStretchScale = 2.2f;
  float bloomPixelScale = 80.0f;
  float bloomThresholdScale = 0.75f;
  int bloomRings = 2;
  int bloomSamplesPerRing = 6;
  float bloomEdgeKeepScale = 0.45f;
  float bloomVeilScale = 0.4f;
  float bloomCreamScale = 0.8f;

  float flareIntensity = 0.08f;
  float flareLength = 0.45f;
  Vec3 flareColour{0.35f, 0.75f, 1.0f};
  float flareThreshold = 0.82f;
  float flareAngle = 0.0f;
  float flareStepDensity = 6.0f;
  float flareSpanScale = 0.75f;
  float flareFalloff = 3.0f;

  float veil = 0.03f;
  float bloomRadius = 0.12f;
  float highlightCream = 0.0f;
  float blackLiftProtection = 0.65f;

  int ghostCount = 0;
  float ghostSpread = 0.35f;
  Vec3 ghostTint{0.55f, 0.8f, 1.0f};
  float ghostIntensity = 0.13f;
  int coatingStyle = 1;
  float coatingWarmResponse = 0.75f;
  float coatingCoolResponse = 1.25f;

  float edgeBlur = 0.05f;
  float tangentialSmear = 0.03f;
  float radialFalloff = 0.65f;
  float edgeBlurPixels = 10.0f;
  float fieldCurvaturePixels = 4.0f;
  float smearPixels = 18.0f;

  float barrel = 0.0f;
  float mustache = 0.0f;
  float verticalCompensation = 0.0f;
  float verticalCompensationScale = 0.35f;

  float closeFocusMumps = 0.0f;
  float faceWidthCompensation = 0.0f;
  float focusDistance = 0.5f;
  float breathingAmount = 0.0f;
  float mumpsScale = 0.28f;

  float lateralCA = 0.03f;
  float longitudinalCA = 0.0f;
  float edgeOnlyCA = 1.0f;
  float lateralCAPixelScale = 4.0f;

  float ovalVignette = 0.05f;
  float vignetteAsymmetry = 0.0f;
  float cornerBias = 0.0f;
  float ovalVignetteScale = 1.8f;
  float vignetteAsymmetryScale = 0.35f;

  float horizontalSmear = 0.03f;
  float verticalSharpness = 0.0f;
  float fieldCurvature = 0.03f;

  float catEyeStrength = 0.0f;
  float bokehVignette = 0.0f;
  float edgeCompression = 0.0f;
  float catEyeDimScale = 0.22f;
  float bokehVignetteDimScale = 0.18f;
  float edgeCompressionScale = 0.16f;
  float centerVeilScale = 0.08f;

  int guidesEnabled = 0;
  int outputAspect = 0;
  float customOutputAspect = 2.39f;
  float safeArea = 0.9f;
  int letterboxPreview = 0;
  float letterboxOpacity = 0.55f;
  float guideAspectStrength = 0.85f;
  float guideSafeStrength = 0.45f;
};

} // namespace rimell
