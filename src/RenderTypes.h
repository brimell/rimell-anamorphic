#pragma once

#include "ofxCore.h"

struct RenderContext {
  void *commandQueue = nullptr;
  void *outputData = nullptr;

  int renderX1 = 0;
  int renderY1 = 0;
  int renderX2 = 0;
  int renderY2 = 0;

  int outputX1 = 0;
  int outputY1 = 0;
  int outputRowFloats = 0;
};
