#pragma once

#include "ofxCore.h"
#include "ofxGPURender.h"
#include "ofxImageEffect.h"

#include <chrono>

namespace rimell {

enum class LogLevel : int {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
  Trace = 4,
};

LogLevel currentLogLevel();
bool shouldLog(LogLevel level);
double slowMsThreshold();

void logMessage(LogLevel level, const char *scope, const char *message);
void logPrintf(LogLevel level, const char *scope, const char *format, ...);

const char *ofxStatusToString(OfxStatus status);

class ScopedLogTimer {
public:
  ScopedLogTimer(LogLevel level, const char *scope, const char *name);
  ~ScopedLogTimer();

  void setResult(const char *result);
  double elapsedMs() const;

private:
  LogLevel level_;
  const char *scope_;
  const char *name_;
  const char *result_;
  std::chrono::steady_clock::time_point started_;
};

} // namespace rimell
