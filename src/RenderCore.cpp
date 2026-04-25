#include "RenderCore.h"

#include "MathUtils.h"
#include "Parameters.h"

#include <algorithm>
#include <cmath>

namespace rimell {

Pixel composeFinalPixel(const Pixel &original, const Pixel &processed, const RenderParams &params) {
  Pixel color = lerpPixel(original, processed, clamp01(params.mix));
  color.a = original.a;
  return color;
}

Pixel applyVignetteAndGuides(Pixel color, float x, float y, int width, int height,
                             const RenderParams &params) {
  const float cx = (static_cast<float>(width) - 1.0f) * 0.5f;
  const float cy = (static_cast<float>(height) - 1.0f) * 0.5f;
  const float nx = (x - cx) / std::max(1.0f, width * 0.5f);
  const float ny = (y - cy) / std::max(1.0f, height * 0.5f);

  const float ovalY = ny * (1.0f + params.ovalVignette * params.ovalVignetteScale);
  const float asym = nx * params.vignetteAsymmetry * params.vignetteAsymmetryScale +
                     ny * params.cornerBias * params.vignetteAsymmetryScale;
  const float vignetteRadius = std::sqrt(nx * nx + ovalY * ovalY) + asym;
  const float vignette = 1.0f - params.ovalVignette * smoothstep(0.35f, 1.2f, vignetteRadius);
  color.r *= vignette;
  color.g *= vignette;
  color.b *= vignette;

  const float edge = smoothstep(0.55f, 1.08f, std::sqrt(nx * nx + ny * ny));
  const float catEye =
      1.0f - params.catEyeStrength * edge * params.catEyeDimScale -
      params.bokehVignette * edge * params.bokehVignetteDimScale;
  color.r *= catEye;
  color.g *= catEye;
  color.b *= catEye;

  if (params.guidesEnabled || params.letterboxPreview) {
    const float target = aspectValue(params.outputAspect, params.customOutputAspect);
    const float current = static_cast<float>(width) / std::max(1.0f, static_cast<float>(height));
    float contentX1 = 0.0f;
    float contentY1 = 0.0f;
    float contentX2 = static_cast<float>(width);
    float contentY2 = static_cast<float>(height);

    if (target > current) {
      const float contentHeight = static_cast<float>(width) / target;
      contentY1 = (static_cast<float>(height) - contentHeight) * 0.5f;
      contentY2 = contentY1 + contentHeight;
    } else {
      const float contentWidth = static_cast<float>(height) * target;
      contentX1 = (static_cast<float>(width) - contentWidth) * 0.5f;
      contentX2 = contentX1 + contentWidth;
    }

    const bool outside = x < contentX1 || x >= contentX2 || y < contentY1 || y >= contentY2;
    if (params.letterboxPreview && outside) {
      const float keep = 1.0f - clamp01(params.letterboxOpacity);
      color.r *= keep;
      color.g *= keep;
      color.b *= keep;
    }

    if (params.guidesEnabled) {
      const float safeInsetX = (contentX2 - contentX1) * (1.0f - params.safeArea) * 0.5f;
      const float safeInsetY = (contentY2 - contentY1) * (1.0f - params.safeArea) * 0.5f;
      const float sx1 = contentX1 + safeInsetX;
      const float sx2 = contentX2 - safeInsetX;
      const float sy1 = contentY1 + safeInsetY;
      const float sy2 = contentY2 - safeInsetY;
      const bool aspectLine = std::abs(x - contentX1) < 1.0f || std::abs(x - contentX2) < 1.0f ||
                              std::abs(y - contentY1) < 1.0f || std::abs(y - contentY2) < 1.0f;
      const bool safeLine = std::abs(x - sx1) < 1.0f || std::abs(x - sx2) < 1.0f ||
                            std::abs(y - sy1) < 1.0f || std::abs(y - sy2) < 1.0f;
      if (aspectLine || safeLine) {
        const float strength = aspectLine ? params.guideAspectStrength : params.guideSafeStrength;
        color.r = lerp(color.r, 1.0f, strength);
        color.g = lerp(color.g, safeLine ? 0.85f : 0.65f, strength);
        color.b = lerp(color.b, 0.25f, strength);
      }
    }
  }

  return color;
}

} // namespace rimell
