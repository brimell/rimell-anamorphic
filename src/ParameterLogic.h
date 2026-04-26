#pragma once

#include "Types.h"

namespace rimell {

enum LookPreset {
  kLookPresetManual = 0,
  kLookPresetCleanScope133 = 1,
  kLookPresetModern18Controlled = 2,
  kLookPresetClassic2xSoftEdge = 3,
  kLookPresetVintage2xBlueStreak = 4,
  kLookPresetWarmCoatedScope = 5,
  kLookPresetNightPracticalFlares = 6,
  kLookPresetLowDistortionCinemaScope = 7,
  kLookPresetSoftBackgroundOvalBokeh = 8,
  kLookPresetWaterfallBokehExperimental = 9,
  kLookPresetHeavyMumpsVintage = 10,
  kLookPresetEdgeSmearExperimental = 11,
  kLookPresetGeometryOnly = 12,
  kLookPresetFlareOnlyControlled = 13,
  kLookPresetRealAnamorphicUtility = 14,
  kLookPresetDebugNeutral = 15,
};

RenderParams clampRenderParams(RenderParams params);
RenderParams applyLookPreset(RenderParams params);
RenderParams normalizeRenderParams(RenderParams params);

} // namespace rimell
