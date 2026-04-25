#include "LensMap.h"

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

  return passed ? 0 : 1;
}
