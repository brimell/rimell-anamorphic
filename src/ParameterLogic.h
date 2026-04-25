#pragma once

#include "Types.h"

namespace rimell {

enum LookPreset {
  kLookPresetManual = 0,
  kLookPresetSubtleModern = 1,
  kLookPresetClassic2x = 2,
  kLookPresetNightFlare = 3,
  kLookPresetGeometryOnly = 4,
};

RenderParams clampRenderParams(RenderParams params);
RenderParams applyLookPreset(RenderParams params);
RenderParams normalizeRenderParams(RenderParams params);

} // namespace rimell
