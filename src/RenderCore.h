#pragma once

#include "Types.h"

namespace rimell {

Pixel composeFinalPixel(const Pixel &original, const Pixel &processed, const RenderParams &params);
Pixel applyVignetteAndGuides(Pixel color, float x, float y, int width, int height,
                             const RenderParams &params);

} // namespace rimell
