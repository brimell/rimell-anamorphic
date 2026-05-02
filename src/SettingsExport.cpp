#include "SettingsExport.h"
#include "Diagnostics.h"
#include <sstream>
#include <iomanip>

namespace rimell {

std::string generateSettingsJson(const RenderParams &params) {
  std::stringstream ss;
  ss << "{\n";
  ss << "  \"schemaVersion\": " << params.schemaVersion << ",\n";
  ss << "  \"mix\": " << params.mix << ",\n";
  ss << "  \"debugView\": " << params.debugView << ",\n";
  ss << "  \"renderQuality\": " << params.renderQuality << ",\n";
  ss << "  \"processingBackend\": " << params.processingBackend << ",\n";
  ss << "  \"lookPreset\": " << params.lookPreset << ",\n";
  ss << "  \"inputMode\": " << params.inputMode << ",\n";
  ss << "  \"squeezeMode\": " << params.squeezeMode << ",\n";
  ss << "  \"anamorphicTransfer\": " << params.anamorphicTransfer << ",\n";
  ss << "  \"lensIdentity\": " << params.lensIdentity << ",\n";
  ss << "  \"squeezeRatio\": " << params.squeezeRatio << ",\n";
  ss << "  \"axisWarp\": " << params.axisWarp << ",\n";
  ss << "  \"centerProtection\": " << params.centerProtection << ",\n";
  ss << "  \"edgeCompressionStart\": " << params.edgeCompressionStart << ",\n";
  ss << "  \"horizontalFovBoost\": " << params.horizontalFovBoost << ",\n";
  ss << "  \"virtualFocalLength\": " << params.virtualFocalLength << ",\n";
  ss << "  \"breathingScale\": " << params.breathingScale << ",\n";
  ss << "  \"bokehStretch\": " << params.bokehStretch << ",\n";
  ss << "  \"bokehRotation\": " << params.bokehRotation << ",\n";
  ss << "  \"bokehEdgeFalloff\": " << params.bokehEdgeFalloff << ",\n";
  ss << "  \"bokehStretchScale\": " << params.bokehStretchScale << ",\n";
  ss << "  \"enableBokeh\": " << params.enableBokeh << ",\n";
  ss << "  \"bokehAmount\": " << params.bokehAmount << ",\n";
  ss << "  \"focusWidth\": " << params.focusWidth << ",\n";
  ss << "  \"focusFalloff\": " << params.focusFalloff << ",\n";
  ss << "  \"maxBokehRadius\": " << params.maxBokehRadius << ",\n";
  ss << "  \"nearBlurAmount\": " << params.nearBlurAmount << ",\n";
  ss << "  \"farBlurAmount\": " << params.farBlurAmount << ",\n";
  ss << "  \"ovalRatio\": " << params.ovalRatio << ",\n";
  ss << "  \"ovalOrientation\": " << params.ovalOrientation << ",\n";
  ss << "  \"ovalAngle\": " << params.ovalAngle << ",\n";
  ss << "  \"invertDepth\": " << params.invertDepth << ",\n";
  ss << "  \"depthBlackPoint\": " << params.depthBlackPoint << ",\n";
  ss << "  \"depthWhitePoint\": " << params.depthWhitePoint << ",\n";
  ss << "  \"depthGamma\": " << params.depthGamma << ",\n";
  ss << "  \"depthSmoothRadius\": " << params.depthSmoothRadius << ",\n";
  ss << "  \"depthEdgeProtect\": " << params.depthEdgeProtect << ",\n";
  ss << "  \"foregroundEdgeProtect\": " << params.foregroundEdgeProtect << ",\n";
  ss << "  \"backgroundEdgeProtect\": " << params.backgroundEdgeProtect << ",\n";
  ss << "  \"occlusionThreshold\": " << params.occlusionThreshold << ",\n";
  ss << "  \"highlightBokehEnable\": " << params.highlightBokehEnable << ",\n";
  ss << "  \"highlightThreshold\": " << params.highlightThreshold << ",\n";
  ss << "  \"highlightSoftness\": " << params.highlightSoftness << ",\n";
  ss << "  \"highlightGain\": " << params.highlightGain << ",\n";
  ss << "  \"highlightRadiusMultiplier\": " << params.highlightRadiusMultiplier << ",\n";
  ss << "  \"highlightSaturation\": " << params.highlightSaturation << ",\n";
  ss << "  \"highlightRolloff\": " << params.highlightRolloff << ",\n";
  ss << "  \"apertureSoftness\": " << params.apertureSoftness << ",\n";
  ss << "  \"rimBrightness\": " << params.rimBrightness << ",\n";
  ss << "  \"centreDensity\": " << params.centreDensity << ",\n";
  ss << "  \"bokehCAEnable\": " << params.bokehCAEnable << ",\n";
  ss << "  \"bokehCAAmount\": " << params.bokehCAAmount << ",\n";
  ss << "  \"catEyeAmount\": " << params.catEyeAmount << ",\n";
  ss << "  \"catEyeStart\": " << params.catEyeStart << ",\n";
  ss << "  \"catEyeCompression\": " << params.catEyeCompression << ",\n";
  ss << "  \"catEyeShift\": " << params.catEyeShift << ",\n";
  ss << "  \"bloomPixelScale\": " << params.bloomPixelScale << ",\n";
  ss << "  \"bloomThresholdScale\": " << params.bloomThresholdScale << ",\n";
  ss << "  \"bloomRings\": " << params.bloomRings << ",\n";
  ss << "  \"bloomSamplesPerRing\": " << params.bloomSamplesPerRing << ",\n";
  ss << "  \"bloomEdgeKeepScale\": " << params.bloomEdgeKeepScale << ",\n";
  ss << "  \"bloomVeilScale\": " << params.bloomVeilScale << ",\n";
  ss << "  \"bloomCreamScale\": " << params.bloomCreamScale << ",\n";
  ss << "  \"flareIntensity\": " << params.flareIntensity << ",\n";
  ss << "  \"flareLength\": " << params.flareLength << "\n";

  ss << "}\n";
  return ss.str();
}

} // namespace rimell
