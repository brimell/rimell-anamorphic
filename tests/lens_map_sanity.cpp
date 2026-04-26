#include "LensMap.h"
#include "ParameterLogic.h"
#include "Parameters.h"

#include <cmath>
#include <iostream>

namespace {

bool near(float a, float b) {
  return std::fabs(a - b) < 0.0001f;
}

bool require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    return false;
  }
  return true;
}

rimell::Image imageFor(int width, int height) {
  rimell::Image image;
  image.bounds.x1 = 0;
  image.bounds.y1 = 0;
  image.bounds.x2 = width;
  image.bounds.y2 = height;
  return image;
}

} // namespace

int main() {
  bool passed = true;
  const rimell::Image image = imageFor(200, 100);

  rimell::RenderParams spherical;
  spherical.anamorphicTransfer = 0.0f;
  const rimell::LensMap sphericalMap = rimell::buildLensMap(180.0f, 50.0f, image, 200, 100, spherical);
  passed = require(near(sphericalMap.sampleCoord.x, sphericalMap.sphericalCoord.x),
                   "spherical transfer zero changed x") &&
           passed;
  passed = require(near(sphericalMap.sampleCoord.y, sphericalMap.sphericalCoord.y),
                   "spherical transfer zero changed y") &&
           passed;

  rimell::RenderParams utility;
  utility.inputMode = 1;
  utility.squeezeMode = 2;
  utility.squeezeRatio = 2.0f;
  utility.anamorphicTransfer = 0.0f;
  const rimell::LensMap utilityMap = rimell::buildLensMap(180.0f, 50.0f, image, 200, 100, utility);
  passed = require(near(utilityMap.sampleCoord.x, utilityMap.sphericalCoord.x / utility.squeezeRatio),
                   "utility desqueeze was affected by transfer or preset identity") &&
           passed;
  passed = require(near(utilityMap.sampleCoord.y, utilityMap.sphericalCoord.y),
                   "utility desqueeze changed y") &&
           passed;

  rimell::RenderParams creative;
  creative.lensIdentity = 2;
  creative.squeezeRatio = 2.0f;
  creative.anamorphicTransfer = 1.0f;
  creative.centerProtection = 0.0f;
  const rimell::LensMap creativeMap = rimell::buildLensMap(195.0f, 50.0f, image, 200, 100, creative);
  passed = require(std::fabs(creativeMap.sampleCoord.x - creativeMap.sphericalCoord.x) > 0.0001f,
                   "creative transfer did not move x") &&
           passed;
  passed = require(creativeMap.transferAmount > 0.0f, "creative transfer amount was zero") && passed;

  rimell::RenderParams autoCrop;
  autoCrop.inputMode = 1;
  autoCrop.squeezeMode = 1;
  autoCrop.squeezeRatio = 2.0f;
  autoCrop.lateralCA = 0.0f;
  autoCrop.autoEdgeCrop = 1;
  const float cropScale = rimell::automaticEdgeCropScale(image, 200, 100, autoCrop);
  passed = require(cropScale > 1.0f, "automatic edge crop did not crop an out-of-bounds map") &&
           passed;
  const rimell::Vec2 croppedEdge = rimell::applyEdgeCrop(199.0f, 50.0f, image, cropScale);
  const rimell::LensMap croppedMap =
      rimell::buildLensMap(croppedEdge.x, croppedEdge.y, image, 200, 100, autoCrop);
  const rimell::Vec2 croppedSource = rimell::lensMapToSourcePixel(croppedMap, image, 200, 100);
  passed = require(croppedSource.x <= 199.0f, "automatic edge crop left right edge out of bounds") &&
           passed;

  rimell::RenderParams protectedCenter;
  protectedCenter.anamorphicTransfer = 1.0f;
  protectedCenter.centerProtection = 1.0f;
  const rimell::LensMap centerMap = rimell::buildLensMap(100.0f, 50.0f, image, 200, 100, protectedCenter);
  passed = require(centerMap.transferAmount < 0.01f, "center protection did not suppress center transfer") &&
           passed;

  rimell::RenderParams edgeCompression;
  edgeCompression.lensIdentity = 0;
  edgeCompression.squeezeRatio = 1.5f;
  edgeCompression.anamorphicTransfer = 1.0f;
  edgeCompression.centerProtection = 0.0f;
  edgeCompression.edgeCompression = 0.8f;
  edgeCompression.edgeCompressionStart = 0.1f;
  const rimell::LensMap edgeMap = rimell::buildLensMap(195.0f, 50.0f, image, 200, 100, edgeCompression);
  passed = require(edgeMap.sampleCoord.x < edgeMap.sphericalCoord.x, "edge compression did not pull right edge inward") &&
           passed;

  rimell::RenderParams squeeze;
  squeeze.inputMode = 1;
  squeeze.squeezeMode = 1;
  squeeze.squeezeRatio = 2.0f;
  const rimell::LensMap squeezeMap = rimell::buildLensMap(180.0f, 50.0f, image, 200, 100, squeeze);
  passed = require(near(squeezeMap.sampleCoord.x, squeezeMap.sphericalCoord.x * squeeze.squeezeRatio),
                   "utility squeeze did not scale x") &&
           passed;

  rimell::RenderParams clamped;
  clamped.renderQuality = 99;
  clamped.lookPreset = 99;
  clamped.bloomRings = 99;
  clamped.bloomSamplesPerRing = 1;
  clamped.ghostCount = 99;
  clamped.coatingStyle = -4;
  clamped = rimell::clampRenderParams(clamped);
  passed = require(clamped.renderQuality == 3 && clamped.lookPreset == rimell::kLookPresetDebugNeutral &&
                       clamped.bloomRings == 8 && clamped.bloomSamplesPerRing == 3 &&
                       clamped.ghostCount == 8 && clamped.coatingStyle == 0,
                   "parameter clamps did not hold expected bounds") &&
           passed;

  rimell::RenderParams cleanScope;
  cleanScope.lookPreset = rimell::kLookPresetCleanScope133;
  cleanScope = rimell::normalizeRenderParams(cleanScope);
  passed = require(cleanScope.lookPreset == rimell::kLookPresetCleanScope133 &&
                       cleanScope.centerProtection > 0.85f && cleanScope.flareIntensity > 0.02f &&
                       cleanScope.ghostCount == 0,
                   "clean scope preset did not become the default creative starting point") &&
           passed;

  rimell::RenderParams night;
  night.lookPreset = rimell::kLookPresetNightPracticalFlares;
  night = rimell::normalizeRenderParams(night);
  passed = require(night.flareIntensity > 0.3f && night.ghostCount == 3 && night.bloomRadius > 0.2f,
                   "night flare preset did not apply expected creative defaults") &&
           passed;

  rimell::RenderParams geometryOnly;
  geometryOnly.lookPreset = rimell::kLookPresetGeometryOnly;
  geometryOnly = rimell::normalizeRenderParams(geometryOnly);
  passed = require(near(geometryOnly.flareIntensity, 0.0f) && near(geometryOnly.lateralCA, 0.0f) &&
                       geometryOnly.ghostCount == 0,
                   "geometry-only preset left additive optical effects enabled") &&
           passed;

  rimell::RenderParams utilityPreset;
  utilityPreset.lookPreset = rimell::kLookPresetRealAnamorphicUtility;
  utilityPreset = rimell::normalizeRenderParams(utilityPreset);
  passed = require(near(utilityPreset.anamorphicTransfer, 0.0f) && near(utilityPreset.flareIntensity, 0.0f) &&
                       near(utilityPreset.edgeCompression, 0.0f) && utilityPreset.squeezeRatio == 2.0f,
                   "real anamorphic utility preset did not suppress creative optics") &&
           passed;

  rimell::RenderParams debugNeutral;
  debugNeutral.lookPreset = rimell::kLookPresetDebugNeutral;
  debugNeutral = rimell::normalizeRenderParams(debugNeutral);
  passed = require(near(debugNeutral.anamorphicTransfer, 0.0f) && near(debugNeutral.flareIntensity, 0.0f) &&
                       near(debugNeutral.squeezeRatio, 1.0f) && debugNeutral.ghostCount == 0,
                   "debug neutral preset did not collapse to a safe baseline") &&
           passed;

  passed = require(near(rimell::aspectValue(0, 3.0f), 2.0f) &&
                       near(rimell::aspectValue(1, 3.0f), 2.39f) &&
                       near(rimell::aspectValue(2, 3.0f), 2.66f) &&
                       near(rimell::aspectValue(3, 3.25f), 3.25f) &&
                       near(rimell::aspectValue(99, 3.0f), 2.39f),
                   "aspect values changed unexpectedly") &&
           passed;

  return passed ? 0 : 1;
}
