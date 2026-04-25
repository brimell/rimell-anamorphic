#include "Describe.h"

#include "Constants.h"
#include "HostSuites.h"
#include "ParameterDefinition.h"

#include "ofxGPURender.h"

namespace rimell {

OfxStatus describe(OfxImageEffectHandle effect) {
  OfxPropertySetHandle props = nullptr;
  gEffectSuite->getPropertySet(effect, &props);

  gPropertySuite->propSetString(props, kOfxPropLabel, 0, "Rimell Anamorphic");
  gPropertySuite->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Rimell/Lens");
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthFloat);
  gPropertySuite->propSetInt(props, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 1);
  gPropertySuite->propSetInt(props, kOfxImageEffectPluginPropSingleInstance, 0, 0);
  gPropertySuite->propSetInt(props, kOfxImageEffectPluginPropHostFrameThreading, 0, 1);
  gPropertySuite->propSetString(props, kOfxImageEffectPluginRenderThreadSafety, 0,
                                kOfxImageEffectRenderFullySafe);
  gPropertySuite->propSetInt(props, kOfxImageEffectPropSupportsTiles, 0, 0);
  gPropertySuite->propSetInt(props, kOfxImageEffectPropTemporalClipAccess, 0, 0);
  gPropertySuite->propSetString(props, kOfxImageEffectPropCPURenderSupported, 0, "true");
  return kOfxStatOK;
}

OfxStatus describeInContext(OfxImageEffectHandle effect) {
  OfxPropertySetHandle props = nullptr;

  gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

  gEffectSuite->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

  gEffectSuite->clipDefine(effect, kDepthClipName, &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthFloat);
  gPropertySuite->propSetInt(props, kOfxImageClipPropOptional, 0, 1);

  OfxParamSetHandle paramSet = nullptr;
  gEffectSuite->getParamSet(effect, &paramSet);

  addGroupParam(paramSet, "coreGroup", "Core", 1);
  addGroupParam(paramSet, "depthGroup", "Depth Map", 1);
  addGroupParam(paramSet, "geometryGroup", "Geometry", 1);
  addGroupParam(paramSet, "highlightGroup", "Highlights / Flares", 0);
  addGroupParam(paramSet, "edgeGroup", "Edge / CA", 0);
  addGroupParam(paramSet, "framingGroup", "Framing", 0);

  addGroupParam(paramSet, "geometryModeGroup", "Mode + Lens", 1);
  addGroupParam(paramSet, "geometryShapeGroup", "Shape + Distortion", 0);
  addGroupParam(paramSet, "geometryFocusGroup", "Focus + Breathing", 0);

  addGroupParam(paramSet, "highlightBloomGroup", "Bloom", 1);
  addGroupParam(paramSet, "highlightFlareGroup", "Flares", 0);
  addGroupParam(paramSet, "highlightGhostGroup", "Ghosting + Coating", 0);
  addGroupParam(paramSet, "highlightToneGroup", "Tone + Veiling", 0);

  addGroupParam(paramSet, "edgeBlurGroup", "Blur + Smear", 1);
  addGroupParam(paramSet, "edgeCaGroup", "Chromatic Aberration", 0);
  addGroupParam(paramSet, "edgeVignetteGroup", "Vignette + Cat Eye", 0);

  addGroupParam(paramSet, "framingAspectGroup", "Aspect Guides", 1);
  addGroupParam(paramSet, "framingLetterboxGroup", "Letterbox + Crop", 0);

  addDoubleParam(paramSet, "mix", "Mix", 1.0, 0.0, 1.0, 0.0, 1.0);
  addChoiceParam(paramSet, "debugView", "Debug View", 0, "Off", "Source", "Depth",
                 "Depth Focus Mask", "Highlight Matte", "Edge Mask");
  addChoiceParam(paramSet, "renderQuality", "Render Quality", 1, "Draft", "Preview", "Final",
                 nullptr, nullptr, "Scales expensive flare, bloom, blur, and chromatic sampling.");
  addChoiceParam(paramSet, "lookPreset", "Look Preset", 0, "Manual", "Subtle Modern",
                 "Classic 2x", "Night Flare", "Geometry Only",
                 "Creative starting points. Manual leaves individual controls unchanged.");

  addChoiceParam(paramSet, "inputMode", "Input Mode", 0, "Spherical -> Anamorphic Look",
                 "Real Anamorphic Utility", "Creative Warp", nullptr,
                 nullptr, "Spherical mode emulates an anamorphic finish from normal circular-lens footage.");
  addChoiceParam(paramSet, "squeezeMode", "Squeeze Mode", 0, "Off", "Squeeze", "Desqueeze",
                 nullptr, nullptr,
                 "Utility geometry for real anamorphic plates or creative warps; ignored by the main spherical look mode.");
  addDoubleParam(paramSet, "anamorphicTransfer", "Anamorphic Transfer", 1.0, 0.0, 1.0, 0.0, 1.0,
                 "Blends from spherical source mapping to the synthetic anamorphic view map.");
  addChoiceParam(paramSet, "lensIdentity", "Lens Identity", 1, "Custom", "Modern 1.33x",
                 "Classic 2x", "Scope Soft Edge", nullptr,
                 "Preset identity used to drive geometry, highlight, flare, and ghost scaling.");
  addBooleanParam(paramSet, "depthMapEnabled", "Use Depth Map", 1,
                  "Connect DaVinci Resolve's AI Depth Map output to the Depth input to drive focus-aware bokeh, blur, flares, ghosts, and longitudinal CA.");
  addBooleanParam(paramSet, "depthMapInvert", "Invert Depth Map", 0);
  addDoubleParam(paramSet, "focusDepth", "Focus Depth", 0.5, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "depthFocusRange", "Depth Focus Range", 0.12, 0.0, 1.0, 0.0, 0.5);
  addDoubleParam(paramSet, "depthInfluence", "Depth Influence", 0.8, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "depthDefocusPixels", "Depth Defocus Pixels", 18.0, 0.0, 160.0, 0.0, 60.0);
  addDoubleParam(paramSet, "depthBloomBoost", "Depth Bloom Boost", 0.65, 0.0, 3.0, 0.0, 1.5);
  // Backward compatibility for older saved projects.
  addDoubleParam(paramSet, "halationExposureThreshold", "Legacy Halation Exposure Threshold", 0.5,
                 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "squeezeRatio", "Squeeze Ratio", 1.33, 1.0, 2.0, 1.0, 2.0);
  addDoubleParam(paramSet, "axisWarp", "Axis Warp", 0.0, 0.0, 1.0, 0.0, 1.0,
                 "Adds user-controlled horizontal/vertical separation on top of the selected lens identity.");
  addDoubleParam(paramSet, "centerProtection", "Center Protection", 0.65, 0.0, 1.0, 0.0, 1.0,
                 "Protects the central subject area from strong anamorphic transfer.");
  addDoubleParam(paramSet, "edgeCompressionStart", "Edge Compression Start", 0.65, 0.0, 1.0, 0.25, 0.95,
                 "Controls where horizontal edge compression begins.");
  addDoubleParam(paramSet, "horizontalFovBoost", "Virtual Horizontal Expansion", 0.0, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "virtualFocalLength", "Virtual Focal Length", 50.0, 10.0, 200.0, 18.0, 100.0);
  addDoubleParam(paramSet, "breathingScale", "Breathing Scale", 0.12, 0.0, 1.0, 0.0, 0.35);

  addDoubleParam(paramSet, "bokehStretch", "Anamorphic Bloom Shape", 0.15, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "bokehRotation", "Bloom Shape Rotation", 0.0, -45.0, 45.0, -15.0, 15.0);
  addDoubleParam(paramSet, "bokehEdgeFalloff", "Bloom Edge Falloff", 0.15, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "bokehStretchScale", "Bloom Stretch Scale", 2.2, 0.0, 8.0, 0.0, 4.0);
  addDoubleParam(paramSet, "bloomPixelScale", "Bloom Pixel Scale", 80.0, 0.0, 300.0, 0.0, 160.0);
  addDoubleParam(paramSet, "bloomThresholdScale", "Bloom Threshold Scale", 0.75, 0.0, 2.0, 0.0, 1.2);
  addIntParam(paramSet, "bloomRings", "Bloom Rings", 2, 1, 8);
  addIntParam(paramSet, "bloomSamplesPerRing", "Bloom Samples Per Ring", 6, 3, 32);
  addDoubleParam(paramSet, "bloomEdgeKeepScale", "Bloom Edge Keep Scale", 0.45, 0.0, 2.0, 0.0, 1.0);
  addDoubleParam(paramSet, "bloomVeilScale", "Bloom Veil Scale", 0.4, 0.0, 4.0, 0.0, 1.5);
  addDoubleParam(paramSet, "bloomCreamScale", "Bloom Cream Scale", 0.8, 0.0, 4.0, 0.0, 2.0);

  addDoubleParam(paramSet, "flareIntensity", "Flare Intensity", 0.08, 0.0, 4.0, 0.0, 1.5);
  addDoubleParam(paramSet, "flareLength", "Flare Length", 0.45, 0.0, 1.0, 0.0, 1.0);
  addRGBParam(paramSet, "flareColour", "Flare Colour", {0.35f, 0.75f, 1.0f});
  addDoubleParam(paramSet, "flareThreshold", "Flare Threshold", 0.82, 0.0, 1.0, 0.4, 1.0);
  addDoubleParam(paramSet, "flareAngle", "Flare Angle", 0.0, -45.0, 45.0, -15.0, 15.0);
  addDoubleParam(paramSet, "flareStepDensity", "Flare Step Density", 6.0, 0.0, 32.0, 0.0, 16.0);
  addDoubleParam(paramSet, "flareSpanScale", "Flare Span Scale", 0.75, 0.0, 3.0, 0.0, 1.5);
  addDoubleParam(paramSet, "flareFalloff", "Flare Falloff", 3.0, 0.0, 12.0, 0.0, 6.0);

  addDoubleParam(paramSet, "veil", "Veil", 0.03, 0.0, 1.0, 0.0, 0.5);
  addDoubleParam(paramSet, "bloomRadius", "Bloom Radius", 0.12, 0.0, 1.0, 0.0, 0.7);
  addDoubleParam(paramSet, "highlightCream", "Highlight Cream", 0.0, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "blackLiftProtection", "Black Lift Protection", 0.65, 0.0, 1.0, 0.0, 1.0);

  addIntParam(paramSet, "ghostCount", "Ghost Count", 0, 0, 8);
  addDoubleParam(paramSet, "ghostSpread", "Ghost Spread", 0.35, 0.0, 1.0, 0.0, 1.0);
  addRGBParam(paramSet, "ghostTint", "Ghost Tint", {0.55f, 0.8f, 1.0f});
  addDoubleParam(paramSet, "ghostIntensity", "Ghost Intensity", 0.13, 0.0, 2.0, 0.0, 0.5);
  addChoiceParam(paramSet, "coatingStyle", "Coating Style", 1, "Warm", "Neutral", "Cool");
  addDoubleParam(paramSet, "coatingWarmResponse", "Warm Coating Response", 0.75, 0.0, 3.0, 0.0, 1.5);
  addDoubleParam(paramSet, "coatingCoolResponse", "Cool Coating Response", 1.25, 0.0, 3.0, 0.0, 1.5);

  addDoubleParam(paramSet, "edgeBlur", "Edge Blur", 0.05, 0.0, 1.0, 0.0, 0.7);
  addDoubleParam(paramSet, "tangentialSmear", "Tangential Smear", 0.03, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "radialFalloff", "Radial Falloff", 0.65, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "edgeBlurPixels", "Edge Blur Pixels", 10.0, 0.0, 80.0, 0.0, 30.0);
  addDoubleParam(paramSet, "fieldCurvaturePixels", "Field Curvature Pixels", 4.0, 0.0, 80.0, 0.0, 30.0);
  addDoubleParam(paramSet, "smearPixels", "Smear Pixels", 18.0, 0.0, 160.0, 0.0, 60.0);

  addDoubleParam(paramSet, "barrel", "Barrel", 0.0, -0.5, 0.5, -0.2, 0.2);
  addDoubleParam(paramSet, "mustache", "Mustache", 0.0, -0.5, 0.5, -0.2, 0.2);
  addDoubleParam(paramSet, "verticalCompensation", "Vertical Compensation", 0.0, -1.0, 1.0, -0.5, 0.5);
  addDoubleParam(paramSet, "verticalCompensationScale", "Vertical Compensation Scale", 0.35, 0.0, 3.0, 0.0, 1.0);

  addDoubleParam(paramSet, "closeFocusMumps", "Close Focus Mumps", 0.0, 0.0, 1.0, 0.0, 0.6);
  addDoubleParam(paramSet, "faceWidthCompensation", "Face Width Compensation", 0.0, 0.0, 1.0, 0.0, 0.6);
  addDoubleParam(paramSet, "focusDistance", "Focus Distance", 0.5, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "breathingAmount", "Breathing Amount", 0.0, -1.0, 1.0, -0.5, 0.5);
  addDoubleParam(paramSet, "mumpsScale", "Mumps Scale", 0.28, 0.0, 3.0, 0.0, 1.0);

  addDoubleParam(paramSet, "lateralCA", "Lateral CA", 0.03, 0.0, 1.0, 0.0, 0.6,
                 "1.0 is approximately four pixels of edge separation.");
  addDoubleParam(paramSet, "longitudinalCA", "Longitudinal CA", 0.0, 0.0, 1.0, 0.0, 0.3,
                 "Adds focus-dependent red/blue channel softness around high-contrast edges.");
  addBooleanParam(paramSet, "edgeOnlyCA", "Edge Only CA", 1);
  addDoubleParam(paramSet, "lateralCAPixelScale", "Lateral CA Pixel Scale", 4.0, 0.0, 32.0, 0.0, 12.0);

  addDoubleParam(paramSet, "ovalVignette", "Oval Vignette", 0.05, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "vignetteAsymmetry", "Asymmetry", 0.0, -1.0, 1.0, -0.5, 0.5);
  addDoubleParam(paramSet, "cornerBias", "Corner Bias", 0.0, -1.0, 1.0, -0.5, 0.5);
  addDoubleParam(paramSet, "ovalVignetteScale", "Oval Vignette Scale", 1.8, 0.0, 8.0, 0.0, 4.0);
  addDoubleParam(paramSet, "vignetteAsymmetryScale", "Vignette Asymmetry Scale", 0.35, 0.0, 3.0, 0.0, 1.0);

  addDoubleParam(paramSet, "horizontalSmear", "Horizontal Smear", 0.03, 0.0, 1.0, 0.0, 0.7);
  addDoubleParam(paramSet, "verticalSharpness", "Vertical Sharpness", 0.0, 0.0, 1.0, 0.0, 0.5);
  addDoubleParam(paramSet, "fieldCurvature", "Field Curvature", 0.03, 0.0, 1.0, 0.0, 0.7);

  addDoubleParam(paramSet, "catEyeStrength", "Edge Highlight Vignette", 0.0, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "bokehVignette", "Bloom Vignette", 0.0, 0.0, 1.0, 0.0, 0.8);
  addDoubleParam(paramSet, "edgeCompression", "Edge Compression", 0.0, 0.0, 1.0, 0.0, 0.7);
  addDoubleParam(paramSet, "catEyeDimScale", "Cat Eye Dim Scale", 0.22, 0.0, 3.0, 0.0, 1.0);
  addDoubleParam(paramSet, "bokehVignetteDimScale", "Bloom Vignette Dim Scale", 0.18, 0.0, 3.0, 0.0, 1.0);
  addDoubleParam(paramSet, "edgeCompressionScale", "Edge Compression Scale", 0.16, 0.0, 3.0, 0.0, 1.0);
  addDoubleParam(paramSet, "centerVeilScale", "Center Veil Scale", 0.08, 0.0, 2.0, 0.0, 0.5);

  addBooleanParam(paramSet, "guidesEnabled", "Aspect Guides", 0);
  addChoiceParam(paramSet, "outputAspect", "Output Aspect", 1, "2.00:1", "2.39:1", "2.66:1", "Custom");
  addDoubleParam(paramSet, "customOutputAspect", "Custom Output Aspect", 2.39, 0.1, 8.0, 1.0, 4.0);
  addDoubleParam(paramSet, "safeArea", "Safe Area", 0.9, 0.5, 1.0, 0.8, 1.0);
  addBooleanParam(paramSet, "letterboxPreview", "Letterbox Preview", 0);
  addDoubleParam(paramSet, "letterboxOpacity", "Letterbox Opacity", 0.55, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "guideAspectStrength", "Aspect Guide Strength", 0.85, 0.0, 1.0, 0.0, 1.0);
  addDoubleParam(paramSet, "guideSafeStrength", "Safe Guide Strength", 0.45, 0.0, 1.0, 0.0, 1.0);
  addBooleanParam(paramSet, "autoEdgeCrop", "Auto Edge Crop", 0,
                  "Crops in by the smallest amount needed to keep warped edge samples inside the source image.");

  const char *core[] = {"mix", "renderQuality", "lookPreset"};
  for (const char *name : core) {
    setParamParent(paramSet, name, "coreGroup");
  }

  const char *depth[] = {"debugView", "depthMapEnabled", "depthMapInvert", "focusDepth",
                         "depthFocusRange", "depthInfluence", "depthDefocusPixels",
                         "depthBloomBoost"};
  for (const char *name : depth) {
    setParamParent(paramSet, name, "depthGroup");
  }
  setParamParent(paramSet, "halationExposureThreshold", "depthGroup");
  {
    OfxParamHandle legacyParam = nullptr;
    OfxPropertySetHandle legacyProps = nullptr;
    if (gParameterSuite->paramGetHandle(paramSet, "halationExposureThreshold", &legacyParam,
                                        &legacyProps) == kOfxStatOK &&
        legacyProps) {
      gPropertySuite->propSetInt(legacyProps, kOfxParamPropSecret, 0, 1);
      gPropertySuite->propSetInt(legacyProps, kOfxParamPropEnabled, 0, 0);
    }
  }

  const char *geometrySubGroups[] = {"geometryModeGroup", "geometryShapeGroup", "geometryFocusGroup"};
  for (const char *name : geometrySubGroups) {
    setParamParent(paramSet, name, "geometryGroup");
  }

  const char *geometryMode[] = {"inputMode", "squeezeMode", "anamorphicTransfer",
                                "lensIdentity", "squeezeRatio", "centerProtection", "edgeCompressionStart",
                                "horizontalFovBoost"};
  for (const char *name : geometryMode) {
    setParamParent(paramSet, name, "geometryModeGroup");
  }

  const char *geometryShape[] = {"axisWarp", "barrel", "mustache", "verticalCompensation",
                                 "verticalCompensationScale", "edgeCompression", "edgeCompressionScale"};
  for (const char *name : geometryShape) {
    setParamParent(paramSet, name, "geometryShapeGroup");
  }

  const char *geometryFocus[] = {"virtualFocalLength", "focusDistance", "breathingAmount",
                                 "breathingScale", "closeFocusMumps", "faceWidthCompensation", "mumpsScale"};
  for (const char *name : geometryFocus) {
    setParamParent(paramSet, name, "geometryFocusGroup");
  }

  const char *highlightSubGroups[] = {"highlightBloomGroup", "highlightFlareGroup",
                                      "highlightGhostGroup", "highlightToneGroup"};
  for (const char *name : highlightSubGroups) {
    setParamParent(paramSet, name, "highlightGroup");
  }

  const char *highlightBloom[] = {"bloomRadius", "bokehStretch", "bokehRotation", "bokehEdgeFalloff",
                                  "bokehStretchScale", "bloomPixelScale", "bloomThresholdScale",
                                  "bloomRings", "bloomSamplesPerRing", "bloomEdgeKeepScale",
                                  "bloomVeilScale", "bloomCreamScale"};
  for (const char *name : highlightBloom) {
    setParamParent(paramSet, name, "highlightBloomGroup");
  }

  const char *highlightFlare[] = {"flareIntensity", "flareLength", "flareColour", "flareThreshold",
                                  "flareAngle", "flareStepDensity", "flareSpanScale", "flareFalloff"};
  for (const char *name : highlightFlare) {
    setParamParent(paramSet, name, "highlightFlareGroup");
  }

  const char *highlightGhost[] = {"ghostCount", "ghostSpread", "ghostTint", "ghostIntensity",
                                  "coatingStyle", "coatingWarmResponse", "coatingCoolResponse"};
  for (const char *name : highlightGhost) {
    setParamParent(paramSet, name, "highlightGhostGroup");
  }

  const char *highlightTone[] = {"veil", "highlightCream", "blackLiftProtection", "centerVeilScale"};
  for (const char *name : highlightTone) {
    setParamParent(paramSet, name, "highlightToneGroup");
  }

  const char *edgeSubGroups[] = {"edgeBlurGroup", "edgeCaGroup", "edgeVignetteGroup"};
  for (const char *name : edgeSubGroups) {
    setParamParent(paramSet, name, "edgeGroup");
  }

  const char *edgeBlur[] = {"edgeBlur", "edgeBlurPixels", "tangentialSmear", "smearPixels",
                            "horizontalSmear", "verticalSharpness", "radialFalloff", "fieldCurvature",
                            "fieldCurvaturePixels"};
  for (const char *name : edgeBlur) {
    setParamParent(paramSet, name, "edgeBlurGroup");
  }

  const char *edgeCa[] = {"lateralCA", "lateralCAPixelScale", "longitudinalCA", "edgeOnlyCA"};
  for (const char *name : edgeCa) {
    setParamParent(paramSet, name, "edgeCaGroup");
  }

  const char *edgeVignette[] = {"ovalVignette", "ovalVignetteScale", "vignetteAsymmetry",
                                "vignetteAsymmetryScale", "cornerBias", "catEyeStrength",
                                "catEyeDimScale", "bokehVignette", "bokehVignetteDimScale"};
  for (const char *name : edgeVignette) {
    setParamParent(paramSet, name, "edgeVignetteGroup");
  }

  const char *framingSubGroups[] = {"framingAspectGroup", "framingLetterboxGroup"};
  for (const char *name : framingSubGroups) {
    setParamParent(paramSet, name, "framingGroup");
  }

  const char *framingAspect[] = {"guidesEnabled", "outputAspect", "customOutputAspect",
                                 "safeArea", "guideAspectStrength", "guideSafeStrength"};
  for (const char *name : framingAspect) {
    setParamParent(paramSet, name, "framingAspectGroup");
  }

  const char *framingLetterbox[] = {"letterboxPreview", "letterboxOpacity", "autoEdgeCrop"};
  for (const char *name : framingLetterbox) {
    setParamParent(paramSet, name, "framingLetterboxGroup");
  }

  return kOfxStatOK;
}

} // namespace rimell
