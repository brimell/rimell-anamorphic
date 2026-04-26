#pragma once

#include "Types.h"

namespace rimell {

enum LookPreset {
  kLookPresetManual = 0,
  kLookPresetSubtleModern = 1,
  kLookPresetClassic2x = 2,
  kLookPresetNightFlare = 3,
  kLookPresetGeometryOnly = 4,
  kLookPresetSoftScope = 5,
  kLookPresetWarmGlass = 6,
  kLookPresetVintageWide = 7,
  kLookPresetCleanPrime = 8,
};

RenderParams clampRenderParams(RenderParams params);
RenderParams applyLookPreset(RenderParams params);
RenderParams normalizeRenderParams(RenderParams params);

} // namespace rimell
