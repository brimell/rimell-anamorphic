#include "ParameterLogic.h"

#include <algorithm>

namespace rimell {
namespace {

template <typename T>
T clampValue(T value, T minValue, T maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

const RenderParams kDefaultLook{};

void resetLookControls(RenderParams &params) {
  params.anamorphicTransfer = kDefaultLook.anamorphicTransfer;
  params.lensIdentity = kDefaultLook.lensIdentity;
  params.squeezeRatio = kDefaultLook.squeezeRatio;
  params.axisWarp = kDefaultLook.axisWarp;
  params.centerProtection = kDefaultLook.centerProtection;
  params.edgeCompressionStart = kDefaultLook.edgeCompressionStart;
  params.horizontalFovBoost = kDefaultLook.horizontalFovBoost;
  params.virtualFocalLength = kDefaultLook.virtualFocalLength;
  params.breathingScale = kDefaultLook.breathingScale;

  params.bokehStretch = kDefaultLook.bokehStretch;
  params.bokehRotation = kDefaultLook.bokehRotation;
  params.bokehEdgeFalloff = kDefaultLook.bokehEdgeFalloff;
  params.bokehStretchScale = kDefaultLook.bokehStretchScale;
  params.enableBokeh = kDefaultLook.enableBokeh;
  params.bokehAmount = kDefaultLook.bokehAmount;
  params.focusWidth = kDefaultLook.focusWidth;
  params.focusFalloff = kDefaultLook.focusFalloff;
  params.maxBokehRadius = kDefaultLook.maxBokehRadius;
  params.nearBlurAmount = kDefaultLook.nearBlurAmount;
  params.farBlurAmount = kDefaultLook.farBlurAmount;
  params.ovalRatio = kDefaultLook.ovalRatio;
  params.ovalOrientation = kDefaultLook.ovalOrientation;
  params.ovalAngle = kDefaultLook.ovalAngle;
  params.invertDepth = kDefaultLook.invertDepth;
  params.depthBlackPoint = kDefaultLook.depthBlackPoint;
  params.depthWhitePoint = kDefaultLook.depthWhitePoint;
  params.depthGamma = kDefaultLook.depthGamma;
  params.depthSmoothRadius = kDefaultLook.depthSmoothRadius;
  params.depthEdgeProtect = kDefaultLook.depthEdgeProtect;
  params.foregroundEdgeProtect = kDefaultLook.foregroundEdgeProtect;
  params.backgroundEdgeProtect = kDefaultLook.backgroundEdgeProtect;
  params.occlusionThreshold = kDefaultLook.occlusionThreshold;
  params.highlightBokehEnable = kDefaultLook.highlightBokehEnable;
  params.highlightThreshold = kDefaultLook.highlightThreshold;
  params.highlightSoftness = kDefaultLook.highlightSoftness;
  params.highlightGain = kDefaultLook.highlightGain;
  params.highlightRadiusMultiplier = kDefaultLook.highlightRadiusMultiplier;
  params.highlightSaturation = kDefaultLook.highlightSaturation;
  params.highlightRolloff = kDefaultLook.highlightRolloff;
  params.apertureSoftness = kDefaultLook.apertureSoftness;
  params.rimBrightness = kDefaultLook.rimBrightness;
  params.centreDensity = kDefaultLook.centreDensity;
  params.bokehCAEnable = kDefaultLook.bokehCAEnable;
  params.bokehCAAmount = kDefaultLook.bokehCAAmount;
  params.catEyeAmount = kDefaultLook.catEyeAmount;
  params.catEyeStart = kDefaultLook.catEyeStart;
  params.catEyeCompression = kDefaultLook.catEyeCompression;
  params.catEyeShift = kDefaultLook.catEyeShift;
  params.bloomPixelScale = kDefaultLook.bloomPixelScale;
  params.bloomThresholdScale = kDefaultLook.bloomThresholdScale;
  params.bloomRings = kDefaultLook.bloomRings;
  params.bloomSamplesPerRing = kDefaultLook.bloomSamplesPerRing;
  params.bloomEdgeKeepScale = kDefaultLook.bloomEdgeKeepScale;
  params.bloomVeilScale = kDefaultLook.bloomVeilScale;
  params.bloomCreamScale = kDefaultLook.bloomCreamScale;

  params.flareIntensity = kDefaultLook.flareIntensity;
  params.flareLength = kDefaultLook.flareLength;
  params.flareColour = kDefaultLook.flareColour;
  params.flareThreshold = kDefaultLook.flareThreshold;
  params.flareAngle = kDefaultLook.flareAngle;
  params.flareStepDensity = kDefaultLook.flareStepDensity;
  params.flareSpanScale = kDefaultLook.flareSpanScale;
  params.flareFalloff = kDefaultLook.flareFalloff;

  params.veil = kDefaultLook.veil;
  params.bloomRadius = kDefaultLook.bloomRadius;
  params.highlightCream = kDefaultLook.highlightCream;
  params.blackLiftProtection = kDefaultLook.blackLiftProtection;

  params.ghostCount = kDefaultLook.ghostCount;
  params.ghostSpread = kDefaultLook.ghostSpread;
  params.ghostTint = kDefaultLook.ghostTint;
  params.ghostIntensity = kDefaultLook.ghostIntensity;
  params.coatingStyle = kDefaultLook.coatingStyle;
  params.coatingWarmResponse = kDefaultLook.coatingWarmResponse;
  params.coatingCoolResponse = kDefaultLook.coatingCoolResponse;

  params.edgeBlur = kDefaultLook.edgeBlur;
  params.tangentialSmear = kDefaultLook.tangentialSmear;
  params.radialFalloff = kDefaultLook.radialFalloff;
  params.edgeBlurPixels = kDefaultLook.edgeBlurPixels;
  params.fieldCurvaturePixels = kDefaultLook.fieldCurvaturePixels;
  params.smearPixels = kDefaultLook.smearPixels;

  params.barrel = kDefaultLook.barrel;
  params.mustache = kDefaultLook.mustache;
  params.verticalCompensation = kDefaultLook.verticalCompensation;
  params.verticalCompensationScale = kDefaultLook.verticalCompensationScale;

  params.closeFocusMumps = kDefaultLook.closeFocusMumps;
  params.faceWidthCompensation = kDefaultLook.faceWidthCompensation;
  params.focusDistance = kDefaultLook.focusDistance;
  params.breathingAmount = kDefaultLook.breathingAmount;
  params.mumpsScale = kDefaultLook.mumpsScale;

  params.lateralCA = kDefaultLook.lateralCA;
  params.longitudinalCA = kDefaultLook.longitudinalCA;
  params.edgeOnlyCA = kDefaultLook.edgeOnlyCA;
  params.lateralCAPixelScale = kDefaultLook.lateralCAPixelScale;

  params.ovalVignette = kDefaultLook.ovalVignette;
  params.vignetteAsymmetry = kDefaultLook.vignetteAsymmetry;
  params.cornerBias = kDefaultLook.cornerBias;
  params.ovalVignetteScale = kDefaultLook.ovalVignetteScale;
  params.vignetteAsymmetryScale = kDefaultLook.vignetteAsymmetryScale;

  params.horizontalSmear = kDefaultLook.horizontalSmear;
  params.verticalSharpness = kDefaultLook.verticalSharpness;
  params.fieldCurvature = kDefaultLook.fieldCurvature;

  params.catEyeStrength = kDefaultLook.catEyeStrength;
  params.bokehVignette = kDefaultLook.bokehVignette;
  params.edgeCompression = kDefaultLook.edgeCompression;
  params.catEyeDimScale = kDefaultLook.catEyeDimScale;
  params.bokehVignetteDimScale = kDefaultLook.bokehVignetteDimScale;
  params.edgeCompressionScale = kDefaultLook.edgeCompressionScale;
  params.centerVeilScale = kDefaultLook.centerVeilScale;
}

void applyCleanScope(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 1;
  params.squeezeRatio = 1.33f;
  params.anamorphicTransfer = 0.42f;
  params.centerProtection = 0.9f;
  params.edgeCompressionStart = 0.72f;
  params.edgeCompression = 0.06f;
  params.bokehStretch = 0.12f;
  params.bokehEdgeFalloff = 0.1f;
  params.edgeBlur = 0.02f;
  params.tangentialSmear = 0.01f;
  params.horizontalSmear = 0.01f;
  params.lateralCA = 0.012f;
  params.veil = 0.015f;
  params.bloomRadius = 0.08f;
  params.highlightCream = 0.03f;
  params.flareIntensity = 0.03f;
  params.flareLength = 0.24f;
  params.flareThreshold = 0.86f;
  params.flareStepDensity = 4.0f;
  params.ghostCount = 0;
  params.ovalVignette = 0.03f;
  params.closeFocusMumps = 0.03f;
  params.faceWidthCompensation = 0.02f;
}

void applyModernControlled(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 1;
  params.squeezeRatio = 1.8f;
  params.anamorphicTransfer = 0.58f;
  params.centerProtection = 0.82f;
  params.edgeCompressionStart = 0.7f;
  params.edgeCompression = 0.11f;
  params.bokehStretch = 0.34f;
  params.bokehEdgeFalloff = 0.18f;
  params.edgeBlur = 0.03f;
  params.tangentialSmear = 0.02f;
  params.horizontalSmear = 0.02f;
  params.fieldCurvature = 0.03f;
  params.lateralCA = 0.02f;
  params.veil = 0.02f;
  params.bloomRadius = 0.1f;
  params.highlightCream = 0.04f;
  params.flareIntensity = 0.05f;
  params.flareLength = 0.3f;
  params.flareThreshold = 0.83f;
  params.ghostCount = 1;
  params.ghostSpread = 0.26f;
  params.ghostIntensity = 0.05f;
  params.ovalVignette = 0.05f;
  params.closeFocusMumps = 0.05f;
}

void applyClassic2xSoftEdge(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 2;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.9f;
  params.centerProtection = 0.58f;
  params.edgeCompressionStart = 0.66f;
  params.edgeCompression = 0.26f;
  params.bokehStretch = 0.42f;
  params.bokehEdgeFalloff = 0.24f;
  params.edgeBlur = 0.08f;
  params.tangentialSmear = 0.08f;
  params.horizontalSmear = 0.06f;
  params.fieldCurvature = 0.06f;
  params.lateralCA = 0.06f;
  params.veil = 0.05f;
  params.bloomRadius = 0.18f;
  params.highlightCream = 0.08f;
  params.flareIntensity = 0.12f;
  params.flareLength = 0.62f;
  params.flareThreshold = 0.8f;
  params.flareStepDensity = 6.0f;
  params.ghostCount = 2;
  params.ghostSpread = 0.34f;
  params.ghostIntensity = 0.12f;
  params.ovalVignette = 0.1f;
  params.closeFocusMumps = 0.12f;
  params.faceWidthCompensation = 0.03f;
}

void applyVintageBlueStreak(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 2;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.98f;
  params.centerProtection = 0.42f;
  params.edgeCompressionStart = 0.6f;
  params.edgeCompression = 0.34f;
  params.bokehStretch = 0.52f;
  params.bokehEdgeFalloff = 0.22f;
  params.edgeBlur = 0.12f;
  params.tangentialSmear = 0.12f;
  params.horizontalSmear = 0.1f;
  params.fieldCurvature = 0.1f;
  params.lateralCA = 0.085f;
  params.veil = 0.08f;
  params.bloomRadius = 0.22f;
  params.highlightCream = 0.12f;
  params.blackLiftProtection = 0.7f;
  params.flareColour = {0.3f, 0.68f, 1.0f};
  params.flareIntensity = 0.22f;
  params.flareLength = 0.84f;
  params.flareThreshold = 0.76f;
  params.flareStepDensity = 8.0f;
  params.ghostCount = 3;
  params.ghostSpread = 0.44f;
  params.ghostIntensity = 0.16f;
  params.ghostTint = {0.72f, 0.86f, 1.0f};
  params.coatingStyle = 2;
  params.coatingCoolResponse = 1.4f;
  params.ovalVignette = 0.14f;
  params.closeFocusMumps = 0.18f;
  params.faceWidthCompensation = 0.02f;
}

void applyWarmCoatedScope(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 3;
  params.squeezeRatio = 1.85f;
  params.anamorphicTransfer = 0.72f;
  params.centerProtection = 0.86f;
  params.edgeCompressionStart = 0.7f;
  params.edgeCompression = 0.1f;
  params.bokehStretch = 0.32f;
  params.bokehEdgeFalloff = 0.18f;
  params.edgeBlur = 0.03f;
  params.tangentialSmear = 0.02f;
  params.horizontalSmear = 0.015f;
  params.lateralCA = 0.02f;
  params.veil = 0.035f;
  params.bloomRadius = 0.14f;
  params.highlightCream = 0.1f;
  params.blackLiftProtection = 0.82f;
  params.flareColour = {0.96f, 0.68f, 0.34f};
  params.flareIntensity = 0.05f;
  params.flareLength = 0.32f;
  params.flareThreshold = 0.84f;
  params.flareStepDensity = 5.0f;
  params.ghostCount = 1;
  params.ghostSpread = 0.24f;
  params.ghostIntensity = 0.06f;
  params.ghostTint = {0.95f, 0.78f, 0.58f};
  params.coatingStyle = 0;
  params.coatingWarmResponse = 1.55f;
  params.coatingCoolResponse = 0.75f;
  params.ovalVignette = 0.04f;
  params.closeFocusMumps = 0.04f;
}

void applyNightPracticalFlares(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 3;
  params.squeezeRatio = 1.8f;
  params.anamorphicTransfer = 0.78f;
  params.centerProtection = 0.68f;
  params.edgeCompressionStart = 0.68f;
  params.edgeCompression = 0.12f;
  params.bokehStretch = 0.46f;
  params.bokehEdgeFalloff = 0.18f;
  params.edgeBlur = 0.05f;
  params.tangentialSmear = 0.04f;
  params.horizontalSmear = 0.03f;
  params.lateralCA = 0.035f;
  params.veil = 0.08f;
  params.bloomRadius = 0.24f;
  params.highlightCream = 0.18f;
  params.blackLiftProtection = 0.88f;
  params.flareIntensity = 0.34f;
  params.flareLength = 0.9f;
  params.flareThreshold = 0.7f;
  params.flareStepDensity = 10.0f;
  params.ghostCount = 3;
  params.ghostSpread = 0.42f;
  params.ghostIntensity = 0.18f;
  params.ghostTint = {0.55f, 0.8f, 1.0f};
  params.ovalVignette = 0.08f;
  params.closeFocusMumps = 0.08f;
}

void applyLowDistortionCinemaScope(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 3;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.34f;
  params.centerProtection = 0.96f;
  params.edgeCompressionStart = 0.78f;
  params.edgeCompression = 0.02f;
  params.bokehStretch = 0.16f;
  params.bokehEdgeFalloff = 0.08f;
  params.edgeBlur = 0.01f;
  params.tangentialSmear = 0.0f;
  params.horizontalSmear = 0.0f;
  params.fieldCurvature = 0.02f;
  params.lateralCA = 0.008f;
  params.veil = 0.0f;
  params.bloomRadius = 0.06f;
  params.highlightCream = 0.02f;
  params.flareIntensity = 0.015f;
  params.flareLength = 0.22f;
  params.ghostCount = 0;
  params.ovalVignette = 0.02f;
  params.closeFocusMumps = 0.01f;
  params.faceWidthCompensation = 0.02f;
  params.mumpsScale = 0.12f;
}

void applySoftBackgroundOvalBokeh(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 1;
  params.squeezeRatio = 1.8f;
  params.anamorphicTransfer = 0.52f;
  params.centerProtection = 0.98f;
  params.edgeCompressionStart = 0.74f;
  params.edgeCompression = 0.06f;
  params.bokehStretch = 0.74f;
  params.bokehEdgeFalloff = 0.36f;
  params.bokehStretchScale = 2.6f;
  params.edgeBlur = 0.04f;
  params.tangentialSmear = 0.02f;
  params.horizontalSmear = 0.02f;
  params.fieldCurvature = 0.04f;
  params.lateralCA = 0.012f;
  params.veil = 0.02f;
  params.bloomRadius = 0.26f;
  params.bloomVeilScale = 0.6f;
  params.bloomCreamScale = 1.0f;
  params.highlightCream = 0.04f;
  params.flareIntensity = 0.02f;
  params.flareLength = 0.18f;
  params.ghostCount = 0;
  params.catEyeStrength = 0.12f;
  params.bokehVignette = 0.1f;
  params.ovalVignette = 0.08f;
  params.closeFocusMumps = 0.02f;
  params.faceWidthCompensation = 0.02f;
}

void applyWaterfallBokehExperimental(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 1;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.66f;
  params.centerProtection = 0.76f;
  params.edgeCompressionStart = 0.7f;
  params.edgeCompression = 0.16f;
  params.bokehStretch = 0.9f;
  params.bokehEdgeFalloff = 0.42f;
  params.bokehStretchScale = 3.0f;
  params.bokehRotation = 2.0f;
  params.edgeBlur = 0.06f;
  params.tangentialSmear = 0.05f;
  params.horizontalSmear = 0.04f;
  params.fieldCurvature = 0.06f;
  params.lateralCA = 0.025f;
  params.veil = 0.03f;
  params.bloomRadius = 0.3f;
  params.highlightCream = 0.08f;
  params.flareIntensity = 0.04f;
  params.flareLength = 0.3f;
  params.ghostCount = 1;
  params.ghostSpread = 0.22f;
  params.ghostIntensity = 0.04f;
  params.catEyeStrength = 0.22f;
  params.bokehVignette = 0.14f;
  params.ovalVignette = 0.1f;
  params.closeFocusMumps = 0.04f;
}

void applyHeavyMumpsVintage(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 2;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.96f;
  params.centerProtection = 0.16f;
  params.edgeCompressionStart = 0.52f;
  params.edgeCompression = 0.42f;
  params.barrel = 0.12f;
  params.mustache = 0.08f;
  params.bokehStretch = 0.46f;
  params.bokehEdgeFalloff = 0.2f;
  params.edgeBlur = 0.14f;
  params.tangentialSmear = 0.15f;
  params.horizontalSmear = 0.12f;
  params.fieldCurvature = 0.12f;
  params.lateralCA = 0.08f;
  params.veil = 0.08f;
  params.bloomRadius = 0.18f;
  params.highlightCream = 0.08f;
  params.flareIntensity = 0.1f;
  params.flareLength = 0.56f;
  params.ghostCount = 2;
  params.ghostSpread = 0.34f;
  params.ghostIntensity = 0.12f;
  params.ovalVignette = 0.12f;
  params.closeFocusMumps = 0.82f;
  params.faceWidthCompensation = 0.12f;
  params.mumpsScale = 1.6f;
}

void applyEdgeSmearExperimental(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 1;
  params.squeezeRatio = 1.7f;
  params.anamorphicTransfer = 0.62f;
  params.centerProtection = 0.42f;
  params.edgeCompressionStart = 0.58f;
  params.edgeCompression = 0.14f;
  params.edgeBlur = 0.18f;
  params.tangentialSmear = 0.22f;
  params.horizontalSmear = 0.2f;
  params.fieldCurvature = 0.16f;
  params.lateralCA = 0.04f;
  params.veil = 0.02f;
  params.bloomRadius = 0.08f;
  params.flareIntensity = 0.02f;
  params.flareLength = 0.18f;
  params.ghostCount = 0;
  params.catEyeStrength = 0.12f;
  params.bokehVignette = 0.08f;
  params.edgeCompressionScale = 0.3f;
  params.centerVeilScale = 0.12f;
  params.ovalVignette = 0.06f;
}

void applyGeometryOnly(RenderParams &params) {
  resetLookControls(params);
  params.mix = 1.0f;
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.guidesEnabled = 0;
  params.letterboxPreview = 0;
  params.lensIdentity = 2;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 1.0f;
  params.centerProtection = 0.62f;
  params.edgeCompressionStart = 0.6f;
  params.edgeCompression = 0.28f;
  params.barrel = 0.05f;
  params.mustache = 0.02f;
  params.bokehStretch = 0.0f;
  params.bokehEdgeFalloff = 0.0f;
  params.bokehStretchScale = 0.0f;
  params.bloomPixelScale = 0.0f;
  params.bloomThresholdScale = 0.0f;
  params.bloomRings = 1;
  params.bloomSamplesPerRing = 3;
  params.bloomEdgeKeepScale = 0.0f;
  params.bloomVeilScale = 0.0f;
  params.bloomCreamScale = 0.0f;
  params.edgeBlur = 0.0f;
  params.tangentialSmear = 0.0f;
  params.radialFalloff = 1.0f;
  params.edgeBlurPixels = 0.0f;
  params.fieldCurvaturePixels = 0.0f;
  params.smearPixels = 0.0f;
  params.horizontalSmear = 0.0f;
  params.verticalSharpness = 0.0f;
  params.fieldCurvature = 0.0f;
  params.lateralCA = 0.0f;
  params.longitudinalCA = 0.0f;
  params.edgeOnlyCA = 0.0f;
  params.lateralCAPixelScale = 0.0f;
  params.veil = 0.0f;
  params.bloomRadius = 0.0f;
  params.highlightCream = 0.0f;
  params.blackLiftProtection = 1.0f;
  params.flareIntensity = 0.0f;
  params.flareLength = 0.0f;
  params.flareStepDensity = 0.0f;
  params.flareSpanScale = 0.0f;
  params.flareFalloff = 0.0f;
  params.ghostCount = 0;
  params.ghostSpread = 0.0f;
  params.ghostIntensity = 0.0f;
  params.coatingWarmResponse = 0.0f;
  params.coatingCoolResponse = 0.0f;
  params.ovalVignette = 0.0f;
  params.vignetteAsymmetry = 0.0f;
  params.cornerBias = 0.0f;
  params.ovalVignetteScale = 0.0f;
  params.vignetteAsymmetryScale = 0.0f;
  params.catEyeStrength = 0.0f;
  params.bokehVignette = 0.0f;
  params.catEyeDimScale = 0.0f;
  params.bokehVignetteDimScale = 0.0f;
  params.centerVeilScale = 0.0f;
  params.edgeCompressionScale = 0.0f;
}

void applyFlareOnlyControlled(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.lensIdentity = 0;
  params.squeezeRatio = 1.0f;
  params.anamorphicTransfer = 0.08f;
  params.centerProtection = 1.0f;
  params.edgeCompressionStart = 1.0f;
  params.edgeCompression = 0.0f;
  params.bokehStretch = 0.0f;
  params.bokehEdgeFalloff = 0.0f;
  params.edgeBlur = 0.0f;
  params.tangentialSmear = 0.0f;
  params.horizontalSmear = 0.0f;
  params.lateralCA = 0.0f;
  params.veil = 0.02f;
  params.bloomRadius = 0.04f;
  params.highlightCream = 0.02f;
  params.flareIntensity = 0.24f;
  params.flareLength = 0.68f;
  params.flareThreshold = 0.8f;
  params.flareStepDensity = 8.0f;
  params.flareSpanScale = 0.9f;
  params.flareFalloff = 3.2f;
  params.ghostCount = 0;
  params.ghostIntensity = 0.0f;
  params.bloomVeilScale = 0.25f;
}

void applyRealAnamorphicUtility(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 1;
  params.squeezeMode = 2;
  params.lensIdentity = 0;
  params.squeezeRatio = 2.0f;
  params.anamorphicTransfer = 0.0f;
  params.centerProtection = 1.0f;
  params.edgeCompressionStart = 1.0f;
  params.edgeCompression = 0.0f;
  params.bokehStretch = 0.0f;
  params.bokehEdgeFalloff = 0.0f;
  params.edgeBlur = 0.0f;
  params.tangentialSmear = 0.0f;
  params.horizontalSmear = 0.0f;
  params.lateralCA = 0.0f;
  params.longitudinalCA = 0.0f;
  params.veil = 0.0f;
  params.bloomRadius = 0.0f;
  params.highlightCream = 0.0f;
  params.flareIntensity = 0.0f;
  params.flareLength = 0.0f;
  params.ghostCount = 0;
  params.ghostIntensity = 0.0f;
  params.ovalVignette = 0.0f;
  params.catEyeStrength = 0.0f;
  params.bokehVignette = 0.0f;
  params.edgeCompressionScale = 0.0f;
  params.guidesEnabled = 1;
}

void applyDebugNeutral(RenderParams &params) {
  resetLookControls(params);
  params.inputMode = 0;
  params.squeezeMode = 0;
  params.guidesEnabled = 0;
  params.letterboxPreview = 0;
  params.lensIdentity = 0;
  params.squeezeRatio = 1.0f;
  params.anamorphicTransfer = 0.0f;
  params.centerProtection = 1.0f;
  params.edgeCompressionStart = 1.0f;
  params.edgeCompression = 0.0f;
  params.horizontalFovBoost = 0.0f;
  params.virtualFocalLength = 50.0f;
  params.breathingScale = 0.0f;
  params.bokehStretch = 0.0f;
  params.bokehEdgeFalloff = 0.0f;
  params.bokehStretchScale = 0.0f;
  params.bloomPixelScale = 0.0f;
  params.bloomThresholdScale = 0.0f;
  params.bloomRings = 1;
  params.bloomSamplesPerRing = 3;
  params.bloomEdgeKeepScale = 0.0f;
  params.bloomVeilScale = 0.0f;
  params.bloomCreamScale = 0.0f;
  params.flareIntensity = 0.0f;
  params.flareLength = 0.0f;
  params.flareThreshold = 1.0f;
  params.flareStepDensity = 0.0f;
  params.flareSpanScale = 0.0f;
  params.flareFalloff = 0.0f;
  params.veil = 0.0f;
  params.bloomRadius = 0.0f;
  params.highlightCream = 0.0f;
  params.blackLiftProtection = 1.0f;
  params.ghostCount = 0;
  params.ghostSpread = 0.0f;
  params.ghostIntensity = 0.0f;
  params.coatingStyle = 1;
  params.coatingWarmResponse = 0.0f;
  params.coatingCoolResponse = 0.0f;
  params.edgeBlur = 0.0f;
  params.tangentialSmear = 0.0f;
  params.radialFalloff = 1.0f;
  params.edgeBlurPixels = 0.0f;
  params.fieldCurvaturePixels = 0.0f;
  params.smearPixels = 0.0f;
  params.barrel = 0.0f;
  params.mustache = 0.0f;
  params.verticalCompensation = 0.0f;
  params.verticalCompensationScale = 0.0f;
  params.closeFocusMumps = 0.0f;
  params.faceWidthCompensation = 0.0f;
  params.focusDistance = 0.5f;
  params.breathingAmount = 0.0f;
  params.mumpsScale = 0.0f;
  params.lateralCA = 0.0f;
  params.longitudinalCA = 0.0f;
  params.edgeOnlyCA = 0.0f;
  params.lateralCAPixelScale = 0.0f;
  params.ovalVignette = 0.0f;
  params.vignetteAsymmetry = 0.0f;
  params.cornerBias = 0.0f;
  params.ovalVignetteScale = 0.0f;
  params.vignetteAsymmetryScale = 0.0f;
  params.horizontalSmear = 0.0f;
  params.verticalSharpness = 0.0f;
  params.fieldCurvature = 0.0f;
  params.catEyeStrength = 0.0f;
  params.bokehVignette = 0.0f;
  params.edgeCompressionScale = 0.0f;
  params.catEyeDimScale = 0.0f;
  params.bokehVignetteDimScale = 0.0f;
  params.edgeCompressionScale = 0.0f;
  params.centerVeilScale = 0.0f;
}

} // namespace

RenderParams clampRenderParams(RenderParams params) {
  params.mix = clampValue(params.mix, 0.0f, 1.0f);
  params.debugView = clampValue(params.debugView, 0, 15);
  params.renderQuality = clampValue(params.renderQuality, 0, 3);
  params.processingBackend = clampValue(params.processingBackend, static_cast<int>(kBackendCpu),
                                        static_cast<int>(kBackendMetalExperimental));
  params.lookPreset = clampValue(params.lookPreset, static_cast<int>(kLookPresetManual),
                                 static_cast<int>(kLookPresetDebugNeutral));
  params.inputMode = clampValue(params.inputMode, 0, 2);
  params.squeezeMode = clampValue(params.squeezeMode, 0, 2);
  params.anamorphicTransfer = clampValue(params.anamorphicTransfer, 0.0f, 1.0f);
  params.lensIdentity = clampValue(params.lensIdentity, 0, 3);
  params.squeezeRatio = clampValue(params.squeezeRatio, 1.0f, 2.0f);
  params.axisWarp = clampValue(params.axisWarp, 0.0f, 1.0f);
  params.centerProtection = clampValue(params.centerProtection, 0.0f, 1.0f);
  params.edgeCompressionStart = clampValue(params.edgeCompressionStart, 0.0f, 1.0f);
  params.bloomRings = clampValue(params.bloomRings, 1, 8);
  params.bloomSamplesPerRing = clampValue(params.bloomSamplesPerRing, 3, 32);
  params.ghostCount = clampValue(params.ghostCount, 0, 8);
  params.coatingStyle = clampValue(params.coatingStyle, 0, 2);
  params.longitudinalCA = clampValue(params.longitudinalCA, 0.0f, 1.0f);
  params.edgeOnlyCA = params.edgeOnlyCA > 0.5f ? 1.0f : 0.0f;
  params.enableBokeh = params.enableBokeh != 0 ? 1 : 0;
  params.bokehAmount = clampValue(params.bokehAmount, 0.0f, 1.0f);
  params.focusWidth = clampValue(params.focusWidth, 0.0f, 1.0f);
  params.focusFalloff = clampValue(params.focusFalloff, 0.0001f, 1.0f);
  params.maxBokehRadius = clampValue(params.maxBokehRadius, 0.0f, 80.0f);
  params.nearBlurAmount = clampValue(params.nearBlurAmount, 0.0f, 2.0f);
  params.farBlurAmount = clampValue(params.farBlurAmount, 0.0f, 2.0f);
  params.ovalRatio = clampValue(params.ovalRatio, 1.0f, 3.0f);
  params.ovalOrientation = clampValue(params.ovalOrientation, 0, 2);
  params.ovalAngle = clampValue(params.ovalAngle, -180.0f, 180.0f);
  params.invertDepth = params.invertDepth != 0 ? 1 : 0;
  params.depthBlackPoint = clampValue(params.depthBlackPoint, 0.0f, 1.0f);
  params.depthWhitePoint = clampValue(params.depthWhitePoint, 0.0f, 1.0f);
  if (std::abs(params.depthWhitePoint - params.depthBlackPoint) < 0.0001f) {
    params.depthWhitePoint = std::min(1.0f, params.depthBlackPoint + 0.0001f);
  }
  params.depthGamma = clampValue(params.depthGamma, 0.05f, 8.0f);
  params.depthSmoothRadius = clampValue(params.depthSmoothRadius, 0, 2);
  params.depthEdgeProtect = clampValue(params.depthEdgeProtect, 0.0f, 40.0f);
  params.foregroundEdgeProtect = clampValue(params.foregroundEdgeProtect, 0.0f, 60.0f);
  params.backgroundEdgeProtect = clampValue(params.backgroundEdgeProtect, 0.0f, 60.0f);
  params.occlusionThreshold = clampValue(params.occlusionThreshold, 0.0f, 0.25f);
  params.highlightBokehEnable = params.highlightBokehEnable != 0 ? 1 : 0;
  params.highlightThreshold = clampValue(params.highlightThreshold, 0.0f, 8.0f);
  params.highlightSoftness = clampValue(params.highlightSoftness, 0.0001f, 4.0f);
  params.highlightGain = clampValue(params.highlightGain, 0.0f, 4.0f);
  params.highlightRadiusMultiplier = clampValue(params.highlightRadiusMultiplier, 0.1f, 4.0f);
  params.highlightSaturation = clampValue(params.highlightSaturation, 0.0f, 2.0f);
  params.highlightRolloff = clampValue(params.highlightRolloff, 0.0f, 4.0f);
  params.apertureSoftness = clampValue(params.apertureSoftness, 0.0f, 0.95f);
  params.rimBrightness = clampValue(params.rimBrightness, 0.0f, 2.0f);
  params.centreDensity = clampValue(params.centreDensity, 0.0f, 2.0f);
  params.bokehCAEnable = params.bokehCAEnable != 0 ? 1 : 0;
  params.bokehCAAmount = clampValue(params.bokehCAAmount, 0.0f, 2.0f);
  params.catEyeAmount = clampValue(params.catEyeAmount, 0.0f, 1.0f);
  params.catEyeStart = clampValue(params.catEyeStart, 0.0f, 1.0f);
  params.catEyeCompression = clampValue(params.catEyeCompression, 0.0f, 2.0f);
  params.catEyeShift = clampValue(params.catEyeShift, 0.0f, 1.0f);
  params.enableDepthMap = params.enableDepthMap != 0 ? 1 : 0;
  params.enableHighlightEffects = params.enableHighlightEffects != 0 ? 1 : 0;
  params.enableEdgeEffects = params.enableEdgeEffects != 0 ? 1 : 0;
  params.enableAdditionalBackgroundBlur = params.enableAdditionalBackgroundBlur != 0 ? 1 : 0;
  params.guidesEnabled = params.guidesEnabled != 0 ? 1 : 0;
  params.outputAspect = clampValue(params.outputAspect, 0, 3);
  params.customOutputAspect = std::max(0.1f, params.customOutputAspect);
  params.safeArea = clampValue(params.safeArea, 0.5f, 1.0f);
  params.letterboxPreview = params.letterboxPreview != 0 ? 1 : 0;
  params.letterboxOpacity = clampValue(params.letterboxOpacity, 0.0f, 1.0f);
  params.autoEdgeCrop = params.autoEdgeCrop != 0 ? 1 : 0;
  params.edgeCropScale = std::max(1.0f, params.edgeCropScale);
  return params;
}

RenderParams applyLookPreset(RenderParams params) {
  bool appliedPreset = true;
  switch (params.lookPreset) {
  case kLookPresetCleanScope133:
    applyCleanScope(params);
    break;
  case kLookPresetModern18Controlled:
    applyModernControlled(params);
    break;
  case kLookPresetClassic2xSoftEdge:
    applyClassic2xSoftEdge(params);
    break;
  case kLookPresetVintage2xBlueStreak:
    applyVintageBlueStreak(params);
    break;
  case kLookPresetWarmCoatedScope:
    applyWarmCoatedScope(params);
    break;
  case kLookPresetNightPracticalFlares:
    applyNightPracticalFlares(params);
    break;
  case kLookPresetLowDistortionCinemaScope:
    applyLowDistortionCinemaScope(params);
    break;
  case kLookPresetSoftBackgroundOvalBokeh:
    applySoftBackgroundOvalBokeh(params);
    break;
  case kLookPresetWaterfallBokehExperimental:
    applyWaterfallBokehExperimental(params);
    break;
  case kLookPresetHeavyMumpsVintage:
    applyHeavyMumpsVintage(params);
    break;
  case kLookPresetEdgeSmearExperimental:
    applyEdgeSmearExperimental(params);
    break;
  case kLookPresetGeometryOnly:
    applyGeometryOnly(params);
    break;
  case kLookPresetFlareOnlyControlled:
    applyFlareOnlyControlled(params);
    break;
  case kLookPresetRealAnamorphicUtility:
    applyRealAnamorphicUtility(params);
    break;
  case kLookPresetDebugNeutral:
    applyDebugNeutral(params);
    break;
  default:
    appliedPreset = false;
    break;
  }
  if (appliedPreset) {
    params.letterboxPreview = 1;
    params.letterboxOpacity = 1.0f;
  }
  return params;
}

RenderParams normalizeRenderParams(RenderParams params) {
  return clampRenderParams(params);
}

float aspectValue(int index, float customOutputAspect) {
  switch (index) {
  case 0:
    return 2.0f;
  case 1:
    return 2.39f;
  case 2:
    return 2.66f;
  case 3:
    return std::max(0.1f, customOutputAspect);
  default:
    return 2.39f;
  }
}

} // namespace rimell
