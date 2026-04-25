#include "ParameterLogic.h"
#include "RenderCore.h"

#include <cmath>
#include <iostream>

namespace {

bool near(float a, float b) {
  return std::fabs(a - b) < 0.0001f;
}

bool samePixel(const rimell::Pixel &a, const rimell::Pixel &b) {
  return near(a.r, b.r) && near(a.g, b.g) && near(a.b, b.b) && near(a.a, b.a);
}

bool require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    return false;
  }
  return true;
}

} // namespace

int main() {
  bool passed = true;

  rimell::RenderParams identityParams;
  identityParams.mix = 0.0f;
  const rimell::Pixel original{0.2f, 0.4f, 0.6f, 0.35f};
  const rimell::Pixel processed{0.9f, 0.1f, 0.0f, 1.0f};
  passed = require(samePixel(rimell::composeFinalPixel(original, processed, identityParams), original),
                   "mix zero did not preserve the original pixel") &&
           passed;

  rimell::RenderParams alphaParams;
  alphaParams.mix = 1.0f;
  const rimell::Pixel alphaResult = rimell::composeFinalPixel(original, processed, alphaParams);
  passed = require(near(alphaResult.r, processed.r) && near(alphaResult.a, original.a),
                   "processed render did not preserve source alpha") &&
           passed;

  rimell::RenderParams clean = rimell::normalizeRenderParams({});
  clean.ovalVignette = 0.0f;
  clean.catEyeStrength = 0.0f;
  clean.bokehVignette = 0.0f;
  passed = require(samePixel(rimell::applyVignetteAndGuides(original, 50.0f, 50.0f, 100, 100, clean), original),
                   "disabled vignette/guides changed the image") &&
           passed;

  rimell::RenderParams letterbox = clean;
  letterbox.letterboxPreview = 1;
  letterbox.letterboxOpacity = 0.5f;
  letterbox.outputAspect = 1;
  const rimell::Pixel dimmed = rimell::applyVignetteAndGuides(original, 50.0f, 0.0f, 100, 100, letterbox);
  passed = require(near(dimmed.r, original.r * 0.5f) && near(dimmed.a, original.a),
                   "letterbox preview did not dim outside content") &&
           passed;

  return passed ? 0 : 1;
}
