#include "Diagnostics.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <mutex>
#include <thread>

namespace rimell {
namespace {

struct LogConfig {
  LogLevel level = LogLevel::Trace;
  double slowMs = 12.0;
  FILE *stream = stderr;
  bool ownsStream = false;
};

std::once_flag gConfigOnce;
std::mutex gWriteMutex;
LogConfig gConfig;

const char *levelName(LogLevel level) {
  switch (level) {
  case LogLevel::Error:
    return "ERROR";
  case LogLevel::Warn:
    return "WARN";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Trace:
    return "TRACE";
  }
  return "UNKNOWN";
}

void initLogConfig() {
  // Always keep diagnostics at maximum verbosity for crash investigations.
  gConfig.level = LogLevel::Trace;

  const char *slowEnv = std::getenv("RIMELL_LOG_SLOW_MS");
  if (slowEnv && *slowEnv) {
    const double value = std::atof(slowEnv);
    if (value > 0.0) {
      gConfig.slowMs = value;
    }
  }

  const char *logPath = std::getenv("RIMELL_LOG_FILE");
  if (!logPath || !*logPath) {
    logPath = "/tmp/RimellAnamorphic.log";
  }

  FILE *file = std::fopen(logPath, "a");
  if (!file) {
    const char *home = std::getenv("HOME");
    if (home && *home) {
      char fallbackPath[PATH_MAX];
      const int written = std::snprintf(fallbackPath,
                                        sizeof(fallbackPath),
                                        "%s/Library/Logs/RimellAnamorphic.log",
                                        home);
      if (written > 0 && static_cast<size_t>(written) < sizeof(fallbackPath)) {
        file = std::fopen(fallbackPath, "a");
      }
    }
  }

  if (file) {
    std::setvbuf(file, nullptr, _IOLBF, 0);
    gConfig.stream = file;
    gConfig.ownsStream = true;
  } else if (gConfig.stream) {
    std::setvbuf(gConfig.stream, nullptr, _IOLBF, 0);
  }
}

void ensureConfig() {
  std::call_once(gConfigOnce, initLogConfig);
}

void writeLogLine(LogLevel level, const char *scope, const char *message) {
  ensureConfig();
  if (!gConfig.stream) {
    return;
  }

  const auto now = std::chrono::system_clock::now();
  const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
  const auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();
  const std::time_t tt = std::chrono::system_clock::to_time_t(now);

  std::tm localTm{};
#if defined(_WIN32)
  localtime_s(&localTm, &tt);
#else
  localtime_r(&tt, &localTm);
#endif

  const auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

  std::lock_guard<std::mutex> lock(gWriteMutex);
  std::fprintf(gConfig.stream,
               "[Rimell][%04d-%02d-%02d %02d:%02d:%02d.%03lld][%s][tid=%zu][%s] %s\n",
               localTm.tm_year + 1900,
               localTm.tm_mon + 1,
               localTm.tm_mday,
               localTm.tm_hour,
               localTm.tm_min,
               localTm.tm_sec,
               static_cast<long long>(millis),
               levelName(level),
               static_cast<size_t>(tid),
               scope ? scope : "general",
               message ? message : "");
  std::fflush(gConfig.stream);
}

} // namespace

LogLevel currentLogLevel() {
  ensureConfig();
  return gConfig.level;
}

bool shouldLog(LogLevel level) {
  ensureConfig();
  return static_cast<int>(level) <= static_cast<int>(gConfig.level);
}

double slowMsThreshold() {
  ensureConfig();
  return gConfig.slowMs;
}

void logMessage(LogLevel level, const char *scope, const char *message) {
  if (!shouldLog(level)) {
    return;
  }
  writeLogLine(level, scope, message);
}

void logPrintf(LogLevel level, const char *scope, const char *format, ...) {
  if (!shouldLog(level) || !format) {
    return;
  }

  char stackBuffer[1024];
  va_list args;
  va_start(args, format);
  const int needed = std::vsnprintf(stackBuffer, sizeof(stackBuffer), format, args);
  va_end(args);

  if (needed < 0) {
    return;
  }

  if (static_cast<size_t>(needed) < sizeof(stackBuffer)) {
    writeLogLine(level, scope, stackBuffer);
    return;
  }

  const size_t size = static_cast<size_t>(needed) + 1;
  char *heapBuffer = static_cast<char *>(std::malloc(size));
  if (!heapBuffer) {
    writeLogLine(level, scope, "failed to allocate log buffer");
    return;
  }

  va_start(args, format);
  std::vsnprintf(heapBuffer, size, format, args);
  va_end(args);
  writeLogLine(level, scope, heapBuffer);
  std::free(heapBuffer);
}

const char *ofxStatusToString(OfxStatus status) {
  switch (status) {
  case kOfxStatOK:
    return "kOfxStatOK";
  case kOfxStatFailed:
    return "kOfxStatFailed";
  case kOfxStatErrFatal:
    return "kOfxStatErrFatal";
  case kOfxStatErrUnknown:
    return "kOfxStatErrUnknown";
  case kOfxStatErrMissingHostFeature:
    return "kOfxStatErrMissingHostFeature";
  case kOfxStatErrUnsupported:
    return "kOfxStatErrUnsupported";
  case kOfxStatErrExists:
    return "kOfxStatErrExists";
  case kOfxStatErrFormat:
    return "kOfxStatErrFormat";
  case kOfxStatErrMemory:
    return "kOfxStatErrMemory";
  case kOfxStatErrBadHandle:
    return "kOfxStatErrBadHandle";
  case kOfxStatErrBadIndex:
    return "kOfxStatErrBadIndex";
  case kOfxStatErrValue:
    return "kOfxStatErrValue";
  case kOfxStatReplyYes:
    return "kOfxStatReplyYes";
  case kOfxStatReplyNo:
    return "kOfxStatReplyNo";
  case kOfxStatReplyDefault:
    return "kOfxStatReplyDefault";
  case kOfxStatErrImageFormat:
    return "kOfxStatErrImageFormat";
  case kOfxStatGLOutOfMemory:
    return "kOfxStatGLOutOfMemory";
  case kOfxStatGPURenderFailed:
    return "kOfxStatGPURenderFailed";
  default:
    return "kOfxStat(unknown)";
  }
}

ScopedLogTimer::ScopedLogTimer(LogLevel level, const char *scope, const char *name)
    : level_(level), scope_(scope), name_(name), result_("ok"),
      started_(std::chrono::steady_clock::now()) {}

ScopedLogTimer::~ScopedLogTimer() {
  const double elapsed = elapsedMs();
  const bool isSlow = elapsed >= slowMsThreshold();
  if (!isSlow && !shouldLog(level_)) {
    return;
  }

  const LogLevel outLevel = isSlow ? LogLevel::Warn : level_;
  logPrintf(outLevel,
            scope_,
            "%s completed in %.3fms result=%s%s",
            name_ ? name_ : "scope",
            elapsed,
            result_ ? result_ : "n/a",
            isSlow ? " [slow]" : "");
}

void ScopedLogTimer::setResult(const char *result) {
  result_ = result;
}

double ScopedLogTimer::elapsedMs() const {
  const auto elapsed = std::chrono::steady_clock::now() - started_;
  return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count();
}

} // namespace rimell
