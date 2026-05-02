#include "SettingsExport.h"
#include "Diagnostics.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace rimell {

OfxStatus exportSettingsToFile(const RenderParams &params, const std::string &filePath) {
  if (filePath.empty()) {
    logPrintf(LogLevel::Warn, "export", "Export file path is empty");
    return kOfxStatErrValue;
  }

  try {
    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
      logPrintf(LogLevel::Error, "export", "Failed to open file for writing: %s", filePath.c_str());
      return kOfxStatFailed;
    }

    file << "# Rimell Anamorphic Settings Export \n";
    file << "[Parameters]\n";
    file << "schemaVersion=" << params.schemaVersion << "\n";
    file << "mix=" << params.mix << "\n";
    file << "debugView=" << params.debugView << "\n";
    file << "renderQuality=" << params.renderQuality << "\n";
    file << "processingBackend=" << params.processingBackend << "\n";
    file << "lookPreset=" << params.lookPreset << "\n";
    file << "inputMode=" << params.inputMode << "\n";
    file << "squeezeMode=" << params.squeezeMode << "\n";
    file << "anamorphicTransfer=" << params.anamorphicTransfer << "\n";
    file << "lensIdentity=" << params.lensIdentity << "\n";
    file << "squeezeRatio=" << params.squeezeRatio << "\n";
    file << "axisWarp=" << params.axisWarp << "\n";
    file << "centerProtection=" << params.centerProtection << "\n";
    file << "edgeCompressionStart=" << params.edgeCompressionStart << "\n";
    file << "horizontalFovBoost=" << params.horizontalFovBoost << "\n";
    file << "virtualFocalLength=" << params.virtualFocalLength << "\n";
    file << "breathingScale=" << params.breathingScale << "\n";
    file << "bokehStretch=" << params.bokehStretch << "\n";
    file << "bokehRotation=" << params.bokehRotation << "\n";
    file << "bokehEdgeFalloff=" << params.bokehEdgeFalloff << "\n";
    file << "bokehStretchScale=" << params.bokehStretchScale << "\n";
    file << "enableBokeh=" << params.enableBokeh << "\n";
    file << "bokehAmount=" << params.bokehAmount << "\n";
    file << "focusWidth=" << params.focusWidth << "\n";
    file << "focusFalloff=" << params.focusFalloff << "\n";
    file << "maxBokehRadius=" << params.maxBokehRadius << "\n";
    file << "nearBlurAmount=" << params.nearBlurAmount << "\n";
    file << "farBlurAmount=" << params.farBlurAmount << "\n";
    file << "ovalRatio=" << params.ovalRatio << "\n";
    file << "ovalOrientation=" << params.ovalOrientation << "\n";
    file << "ovalAngle=" << params.ovalAngle << "\n";
    file << "invertDepth=" << params.invertDepth << "\n";
    file << "depthBlackPoint=" << params.depthBlackPoint << "\n";
    file << "depthWhitePoint=" << params.depthWhitePoint << "\n";
    file << "depthGamma=" << params.depthGamma << "\n";
    file << "depthSmoothRadius=" << params.depthSmoothRadius << "\n";
    file << "depthEdgeProtect=" << params.depthEdgeProtect << "\n";
    file << "foregroundEdgeProtect=" << params.foregroundEdgeProtect << "\n";
    file << "backgroundEdgeProtect=" << params.backgroundEdgeProtect << "\n";
    file << "occlusionThreshold=" << params.occlusionThreshold << "\n";
    file << "highlightBokehEnable=" << params.highlightBokehEnable << "\n";
    file << "highlightThreshold=" << params.highlightThreshold << "\n";
    file << "highlightSoftness=" << params.highlightSoftness << "\n";
    file << "highlightGain=" << params.highlightGain << "\n";
    file << "highlightRadiusMultiplier=" << params.highlightRadiusMultiplier << "\n";
    file << "highlightSaturation=" << params.highlightSaturation << "\n";
    file << "highlightRolloff=" << params.highlightRolloff << "\n";
    file << "apertureSoftness=" << params.apertureSoftness << "\n";
    file << "rimBrightness=" << params.rimBrightness << "\n";
    file << "centreDensity=" << params.centreDensity << "\n";
    file << "bokehCAEnable=" << params.bokehCAEnable << "\n";
    file << "bokehCAAmount=" << params.bokehCAAmount << "\n";
    file << "catEyeAmount=" << params.catEyeAmount << "\n";
    file << "catEyeStart=" << params.catEyeStart << "\n";
    file << "catEyeCompression=" << params.catEyeCompression << "\n";
    file << "catEyeShift=" << params.catEyeShift << "\n";
    file << "bloomPixelScale=" << params.bloomPixelScale << "\n";
    file << "bloomThresholdScale=" << params.bloomThresholdScale << "\n";
    file << "bloomRings=" << params.bloomRings << "\n";
    file << "bloomSamplesPerRing=" << params.bloomSamplesPerRing << "\n";
    file << "bloomEdgeKeepScale=" << params.bloomEdgeKeepScale << "\n";
    file << "bloomVeilScale=" << params.bloomVeilScale << "\n";
    file << "bloomCreamScale=" << params.bloomCreamScale << "\n";
    file << "flareIntensity=" << params.flareIntensity << "\n";
    file << "flareLength=" << params.flareLength << "\n";

    file.close();
    logPrintf(LogLevel::Info, "export", "Settings exported to: %s", filePath.c_str());
    return kOfxStatOK;
  } catch (const std::exception &ex) {
    logPrintf(LogLevel::Error, "export", "Exception during export: %s", ex.what());
    return kOfxStatErrUnknown;
  }
}

} // namespace rimell
