#pragma once

namespace rimell {

inline float lensPresetAxisWarp(int lensIdentity) {
  switch (lensIdentity) {
  case 1:
    return 0.22f;
  case 2:
    return 0.62f;
  case 3:
    return 0.46f;
  default:
    return 0.0f;
  }
}

inline float lensPresetBloomScale(int lensIdentity) {
  switch (lensIdentity) {
  case 1:
    return 1.08f;
  case 2:
    return 1.45f;
  case 3:
    return 1.32f;
  default:
    return 1.0f;
  }
}

inline float lensPresetFlareScale(int lensIdentity) {
  switch (lensIdentity) {
  case 1:
    return 1.08f;
  case 2:
    return 1.55f;
  case 3:
    return 1.25f;
  default:
    return 1.0f;
  }
}

inline float lensPresetGhostScaleX(int lensIdentity) {
  switch (lensIdentity) {
  case 2:
    return 1.18f;
  case 3:
    return 1.1f;
  default:
    return 1.04f;
  }
}

inline float lensPresetGhostScaleY(int lensIdentity) {
  switch (lensIdentity) {
  case 2:
    return 0.9f;
  case 3:
    return 0.94f;
  default:
    return 0.98f;
  }
}

} // namespace rimell
