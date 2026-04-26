#include "Describe.h"

#include "Constants.h"
#include "HostSuites.h"
#include "ParameterDefinition.h"

#include "ofxGPURender.h"

#include <cstddef>

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
#if defined(__APPLE__)
  gPropertySuite->propSetString(props, kOfxImageEffectPropCPURenderSupported, 0, "false");
  gPropertySuite->propSetString(props, kOfxImageEffectPropMetalRenderSupported, 0, "true");
#else
  gPropertySuite->propSetString(props, kOfxImageEffectPropCPURenderSupported, 0, "true");
#endif
  return kOfxStatOK;
}

OfxStatus describeInContext(OfxImageEffectHandle effect) {
  OfxPropertySetHandle props = nullptr;

  gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

  gEffectSuite->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

  gEffectSuite->clipDefine(effect, "Depth", &props);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
  gPropertySuite->propSetString(props, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentAlpha);
  gPropertySuite->propSetInt(props, kOfxImageClipPropOptional, 0, 1);

  OfxParamSetHandle paramSet = nullptr;
  gEffectSuite->getParamSet(effect, &paramSet);

  // Top-level panel sections.
  addPageParam(paramSet, "corePage", "Core");
  addPageParam(paramSet, "geometryPage", "Geometry");
  addPageParam(paramSet, "highlightPage", "Highlights / Flares");
  addPageParam(paramSet, "edgePage", "Edge / CA");
  addPageParam(paramSet, "framingPage", "Framing");

    // Collapsible groups.
    addGroupParam(paramSet, "coreGroup", "Core", true);
    addGroupParam(paramSet, "geometryGroup", "Geometry", true);
    addGroupParam(paramSet, "highlightsGroup", "Highlights / Flares", false);
    addGroupParam(paramSet, "edgeCaGroup", "Edge / CA", false);
    addGroupParam(paramSet, "framingGroup", "Framing", false);
    addGroupParam(paramSet, "performanceGroup", "Performance", false);
    addGroupParam(paramSet, "debugGroup", "Debug", false);

  // Core controls.
    addDoubleParam(paramSet,
              "mix",
              "Mix",
              1.0,
              0.0,
              1.0,
              0.0,
              1.0,
              "Global strength of the effect.",
              "coreGroup");
  addChoiceParam(paramSet,
                 "debugView",
                 "Debug View",
                 0,
                 {"Off",
                  "Source",
                  "Highlight Matte",
                  "Edge Mask",
                  "Metal Identity",
                  "Metal Bilinear",
              "Metal Basic Geometry"},
              nullptr,
              "debugGroup");
  addChoiceParam(paramSet, "processingBackend", "Processing Backend", kBackendAuto,
                 {"CPU", "Auto", "Metal Experimental"},
              "Auto is the default on Apple and may use Metal only when the render format and host state are known-good.",
              "performanceGroup");
  addChoiceParam(paramSet, "renderQuality", "Render Quality", 1, {"Draft", "Preview", "Final", "Ultra"},
              "Caps expensive flare, bloom, blur, ghost, and chromatic sampling for playback or export.",
              "performanceGroup");
  addChoiceParam(paramSet, "lookPreset", "Look Preset", 0,
                 {"Manual", "Clean Scope 1.33x", "Modern 1.8x Controlled", "Classic 2x Soft Edge",
                  "Vintage 2x Blue Streak", "Warm Coated Scope", "Night Practical Flares",
                  "Low Distortion CinemaScope", "Soft Background Oval Bokeh",
                  "Waterfall Bokeh Experimental", "Heavy Mumps Vintage", "Edge Smear Experimental",
                  "Geometry Only", "Flare Only Controlled", "Real Anamorphic Utility", "Debug / Neutral"},
              "Creative starting points. Choosing one writes its values into the controls, then returns to Manual.",
              "coreGroup");
  addIntParam(paramSet, "schemaVersion", "Schema Version", kCurrentParameterSchemaVersion,
            kCurrentParameterSchemaVersion, kCurrentParameterSchemaVersion, nullptr, "debugGroup");
  setParamFlags(paramSet, "schemaVersion", 1, 0, 1, 0);

  addChoiceParam(paramSet, "inputMode", "Input Mode", 0,
                 {"Spherical -> Anamorphic Look", "Real Anamorphic Utility", "Creative Warp"},
              "Spherical mode emulates an anamorphic finish from normal circular-lens footage.",
              "coreGroup");
  addChoiceParam(paramSet, "squeezeMode", "Squeeze Mode", 0, {"Off", "Squeeze", "Desqueeze"},
              "Utility geometry for real anamorphic plates or creative warps; ignored by the main spherical look mode.",
              "geometryGroup");
  addDoubleParam(paramSet, "anamorphicTransfer", "Anamorphic Transfer", 1.0, 0.0, 1.0, 0.0, 1.0,
              "Blends from spherical source mapping to the synthetic anamorphic view map.", "geometryGroup");
  addChoiceParam(paramSet, "lensIdentity", "Lens Identity", 1, {"Custom", "Modern 1.33x",
                 "Classic 2x", "Scope Soft Edge"},
              "Preset identity used to drive geometry, highlight, flare, and ghost scaling.", "geometryGroup");
  // Backward compatibility for older saved projects.
  addDoubleParam(paramSet, "halationExposureThreshold", "Legacy Halation Exposure Threshold", 0.5,
              0.0, 1.0, 0.0, 1.0, nullptr, "debugGroup");
  // Geometry controls.
    addDoubleParam(paramSet, "squeezeRatio", "Squeeze Ratio", 1.33, 1.0, 2.0, 1.0, 2.0, nullptr, "geometryGroup");
  addDoubleParam(paramSet, "axisWarp", "Axis Warp", 0.0, 0.0, 1.0, 0.0, 1.0,
              "Adds user-controlled horizontal/vertical separation on top of the selected lens identity.", "geometryGroup");
  addDoubleParam(paramSet, "centerProtection", "Center Protection", 0.65, 0.0, 1.0, 0.0, 1.0,
              "Protects the central subject area from strong anamorphic transfer.", "geometryGroup");
  addDoubleParam(paramSet, "edgeCompressionStart", "Edge Compression Start", 0.65, 0.0, 1.0, 0.25, 0.95,
              "Controls where horizontal edge compression begins.", "geometryGroup");
    addDoubleParam(paramSet, "horizontalFovBoost", "Virtual Horizontal Expansion", 0.0, 0.0, 1.0, 0.0, 1.0,
              nullptr, "geometryGroup");
    addDoubleParam(paramSet, "virtualFocalLength", "Virtual Focal Length", 50.0, 10.0, 200.0, 18.0, 100.0,
              nullptr, "geometryGroup");
    addDoubleParam(paramSet, "breathingScale", "Breathing Scale", 0.12, 0.0, 1.0, 0.0, 0.35,
              nullptr, "geometryGroup");

  // Highlights and flares.
    addDoubleParam(paramSet, "bloomRadius", "Bloom Radius", 0.12, 0.0, 1.0, 0.0, 0.7, nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bokehStretch", "Anamorphic Bloom Shape", 0.15, 0.0, 1.0, 0.0, 1.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bokehRotation", "Bloom Shape Rotation", 0.0, -45.0, 45.0, -15.0, 15.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bokehEdgeFalloff", "Bloom Edge Falloff", 0.15, 0.0, 1.0, 0.0, 1.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bokehStretchScale", "Bloom Stretch Scale", 2.2, 0.0, 8.0, 0.0, 4.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bloomPixelScale", "Bloom Pixel Scale", 80.0, 0.0, 300.0, 0.0, 160.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bloomThresholdScale", "Bloom Threshold Scale", 0.75, 0.0, 2.0, 0.0, 1.2,
              nullptr, "highlightsGroup");
    addIntParam(paramSet, "bloomRings", "Bloom Rings", 2, 1, 8, nullptr, "highlightsGroup");
    addIntParam(paramSet, "bloomSamplesPerRing", "Bloom Samples Per Ring", 6, 3, 32, nullptr,
            "highlightsGroup");
    addDoubleParam(paramSet, "bloomEdgeKeepScale", "Bloom Edge Keep Scale", 0.45, 0.0, 2.0, 0.0, 1.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bloomVeilScale", "Bloom Veil Scale", 0.4, 0.0, 4.0, 0.0, 1.5,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "bloomCreamScale", "Bloom Cream Scale", 0.8, 0.0, 4.0, 0.0, 2.0,
              nullptr, "highlightsGroup");

    addDoubleParam(paramSet,
              "flareIntensity",
              "Flare Intensity",
              0.08,
              0.0,
              4.0,
              0.0,
              1.5,
              "Strength of highlight-driven streak flares.",
              "highlightsGroup");
    addDoubleParam(paramSet, "flareLength", "Flare Length", 0.45, 0.0, 1.0, 0.0, 1.0, nullptr, "highlightsGroup");
    addRGBParam(paramSet, "flareColour", "Flare Colour", {0.35f, 0.75f, 1.0f}, nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "flareThreshold", "Flare Threshold", 0.82, 0.0, 1.0, 0.4, 1.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "flareAngle", "Flare Angle", 0.0, -45.0, 45.0, -15.0, 15.0, nullptr,
              "highlightsGroup");
    addDoubleParam(paramSet, "flareStepDensity", "Flare Step Density", 6.0, 0.0, 32.0, 0.0, 16.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "flareSpanScale", "Flare Span Scale", 0.75, 0.0, 3.0, 0.0, 1.5,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "flareFalloff", "Flare Falloff", 3.0, 0.0, 12.0, 0.0, 6.0, nullptr, "highlightsGroup");

    addDoubleParam(paramSet, "veil", "Veil", 0.03, 0.0, 1.0, 0.0, 0.5, nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "highlightCream", "Highlight Cream", 0.0, 0.0, 1.0, 0.0, 1.0,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "blackLiftProtection", "Black Lift Protection", 0.65, 0.0, 1.0, 0.0, 1.0,
              nullptr, "highlightsGroup");
    addBooleanParam(paramSet, "enableHighlightEffects", "Enable Highlight Effects", 1,
          "Master enable for bloom, flare, ghosts, and center veil additives.", "highlightsGroup");
    addBooleanParam(paramSet, "enableAdditionalBackgroundBlur", "Enable Additional Background Blur", 0,
          "Applies a soft depth-aware background blur pass behind in-focus subjects.", "highlightsGroup");

    addIntParam(paramSet, "ghostCount", "Ghost Count", 0, 0, 8, nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "ghostSpread", "Ghost Spread", 0.35, 0.0, 1.0, 0.0, 1.0, nullptr,
              "highlightsGroup");
    addRGBParam(paramSet, "ghostTint", "Ghost Tint", {0.55f, 0.8f, 1.0f}, nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "ghostIntensity", "Ghost Intensity", 0.13, 0.0, 2.0, 0.0, 0.5,
              nullptr, "highlightsGroup");
    addChoiceParam(paramSet, "coatingStyle", "Coating Style", 1, {"Warm", "Neutral", "Cool"}, nullptr,
              "highlightsGroup");
    addDoubleParam(paramSet, "coatingWarmResponse", "Warm Coating Response", 0.75, 0.0, 3.0, 0.0, 1.5,
              nullptr, "highlightsGroup");
    addDoubleParam(paramSet, "coatingCoolResponse", "Cool Coating Response", 1.25, 0.0, 3.0, 0.0, 1.5,
              nullptr, "highlightsGroup");

  // Edge treatment and chromatic aberration.
    addBooleanParam(paramSet, "enableEdgeEffects", "Enable Edge Effects", 1,
              "Master enable for edge blur, tangential smear, field curvature, and vertical sharpness.",
              "edgeCaGroup");
    addDoubleParam(paramSet, "edgeBlur", "Edge Blur", 0.05, 0.0, 1.0, 0.0, 0.7, nullptr, "edgeCaGroup");
    addDoubleParam(paramSet, "tangentialSmear", "Tangential Smear", 0.03, 0.0, 1.0, 0.0, 0.8,
              nullptr, "edgeCaGroup");
    addDoubleParam(paramSet, "radialFalloff", "Radial Falloff", 0.65, 0.0, 1.0, 0.0, 1.0,
              nullptr, "edgeCaGroup");
    addDoubleParam(paramSet, "edgeBlurPixels", "Edge Blur Pixels", 10.0, 0.0, 80.0, 0.0, 30.0,
              nullptr, "edgeCaGroup");
    addDoubleParam(paramSet, "fieldCurvaturePixels", "Field Curvature Pixels", 4.0, 0.0, 80.0, 0.0, 30.0,
              nullptr, "edgeCaGroup");
    addDoubleParam(paramSet, "smearPixels", "Smear Pixels", 18.0, 0.0, 160.0, 0.0, 60.0,
              nullptr, "edgeCaGroup");

    addDoubleParam(paramSet, "barrel", "Barrel", 0.0, -0.5, 0.5, -0.2, 0.2, nullptr, "geometryGroup");
    addDoubleParam(paramSet, "mustache", "Mustache", 0.0, -0.5, 0.5, -0.2, 0.2, nullptr, "geometryGroup");
    addDoubleParam(paramSet, "verticalCompensation", "Vertical Compensation", 0.0, -1.0, 1.0, -0.5, 0.5,
              nullptr, "geometryGroup");
    addDoubleParam(paramSet, "verticalCompensationScale", "Vertical Compensation Scale", 0.35, 0.0, 3.0, 0.0, 1.0,
              nullptr, "geometryGroup");

    addDoubleParam(paramSet, "closeFocusMumps", "Close Focus Mumps", 0.0, 0.0, 1.0, 0.0, 0.6,
              nullptr, "geometryGroup");
    addDoubleParam(paramSet, "faceWidthCompensation", "Face Width Compensation", 0.0, 0.0, 1.0, 0.0, 0.6,
              nullptr, "geometryGroup");
    addDoubleParam(paramSet, "focusDistance", "Focus Distance", 0.5, 0.0, 1.0, 0.0, 1.0,
              nullptr, "geometryGroup");
    addDoubleParam(paramSet, "breathingAmount", "Breathing Amount", 0.0, -1.0, 1.0, -0.5, 0.5,
              nullptr, "geometryGroup");
    addDoubleParam(paramSet, "mumpsScale", "Mumps Scale", 0.28, 0.0, 3.0, 0.0, 1.0,
              nullptr, "geometryGroup");

  addDoubleParam(paramSet, "lateralCA", "Lateral CA", 0.03, 0.0, 1.0, 0.0, 0.6,
                 "1.0 is approximately four pixels of edge separation.", "edgeCaGroup");
  addDoubleParam(paramSet, "longitudinalCA", "Longitudinal CA", 0.0, 0.0, 1.0, 0.0, 0.3,
                 "Adds focus-dependent red/blue channel softness around high-contrast edges.", "edgeCaGroup");
  addBooleanParam(paramSet, "edgeOnlyCA", "Edge Only CA", 1, nullptr, "edgeCaGroup");
  addDoubleParam(paramSet, "lateralCAPixelScale", "Lateral CA Pixel Scale", 4.0, 0.0, 32.0, 0.0, 12.0,
                 nullptr, "edgeCaGroup");

  addDoubleParam(paramSet, "ovalVignette", "Oval Vignette", 0.05, 0.0, 1.0, 0.0, 0.8, nullptr, "framingGroup");
  addDoubleParam(paramSet, "vignetteAsymmetry", "Asymmetry", 0.0, -1.0, 1.0, -0.5, 0.5,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "cornerBias", "Corner Bias", 0.0, -1.0, 1.0, -0.5, 0.5,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "ovalVignetteScale", "Oval Vignette Scale", 1.8, 0.0, 8.0, 0.0, 4.0,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "vignetteAsymmetryScale", "Vignette Asymmetry Scale", 0.35, 0.0, 3.0, 0.0, 1.0,
                 nullptr, "framingGroup");

  addDoubleParam(paramSet, "horizontalSmear", "Horizontal Smear", 0.03, 0.0, 1.0, 0.0, 0.7,
                 nullptr, "edgeCaGroup");
  addDoubleParam(paramSet, "verticalSharpness", "Vertical Sharpness", 0.0, 0.0, 1.0, 0.0, 0.5,
                 nullptr, "edgeCaGroup");
  addDoubleParam(paramSet, "fieldCurvature", "Field Curvature", 0.03, 0.0, 1.0, 0.0, 0.7,
                 nullptr, "edgeCaGroup");

  addDoubleParam(paramSet, "catEyeStrength", "Edge Highlight Vignette", 0.0, 0.0, 1.0, 0.0, 0.8,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "bokehVignette", "Bloom Vignette", 0.0, 0.0, 1.0, 0.0, 0.8,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "edgeCompression", "Edge Compression", 0.0, 0.0, 1.0, 0.0, 0.7,
                 nullptr, "geometryGroup");
  addDoubleParam(paramSet, "catEyeDimScale", "Cat Eye Dim Scale", 0.22, 0.0, 3.0, 0.0, 1.0,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "bokehVignetteDimScale", "Bloom Vignette Dim Scale", 0.18, 0.0, 3.0, 0.0, 1.0,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "edgeCompressionScale", "Edge Compression Scale", 0.16, 0.0, 3.0, 0.0, 1.0,
                 nullptr, "geometryGroup");
  addDoubleParam(paramSet, "centerVeilScale", "Center Veil Scale", 0.08, 0.0, 2.0, 0.0, 0.5,
                 nullptr, "highlightsGroup");

  // Framing and guide overlays.
  addBooleanParam(paramSet, "guidesEnabled", "Aspect Guides", 0, nullptr, "framingGroup");
  addChoiceParam(paramSet, "outputAspect", "Output Aspect", 1, {"2.00:1", "2.39:1", "2.66:1", "Custom"},
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "customOutputAspect", "Custom Output Aspect", 2.39, 0.1, 8.0, 1.0, 4.0,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "safeArea", "Safe Area", 0.9, 0.5, 1.0, 0.8, 1.0, nullptr, "framingGroup");
  addBooleanParam(paramSet, "letterboxPreview", "Letterbox Preview", 0, nullptr, "framingGroup");
  addDoubleParam(paramSet, "letterboxOpacity", "Letterbox Opacity", 0.55, 0.0, 1.0, 0.0, 1.0,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "guideAspectStrength", "Aspect Guide Strength", 0.85, 0.0, 1.0, 0.0, 1.0,
                 nullptr, "framingGroup");
  addDoubleParam(paramSet, "guideSafeStrength", "Safe Guide Strength", 0.45, 0.0, 1.0, 0.0, 1.0,
                 nullptr, "framingGroup");
  addBooleanParam(paramSet, "autoEdgeCrop", "Auto Edge Crop", 0,
                  "Crops in by the smallest amount needed to keep warped edge samples inside the source image.",
                  "framingGroup");

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

  addPageChild(paramSet, "corePage", "coreGroup");
  addPageChild(paramSet, "corePage", "performanceGroup");
  addPageChild(paramSet, "corePage", "debugGroup");
  addPageChild(paramSet, "geometryPage", "geometryGroup");
  addPageChild(paramSet, "highlightPage", "highlightsGroup");
  addPageChild(paramSet, "edgePage", "edgeCaGroup");
  addPageChild(paramSet, "framingPage", "framingGroup");

  return kOfxStatOK;
}

} // namespace rimell
