#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_FUNCTION __PRETTY_FUNCTION__

#if defined(STPYV8_LOG_LEVEL)
// change SPDLOG_ACTIVE_LEVEL based on STPYV8_LOG_LEVEL passed from build system
#undef SPDLOG_ACTIVE_LEVEL
#if STPYV8_LOG_LEVEL == TRACE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#elif STPYV8_LOG_LEVEL == DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#elif STPYV8_LOG_LEVEL == INFO
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#elif STPYV8_LOG_LEVEL == WARN
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_WARN
#elif STPYV8_LOG_LEVEL == ERROR
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#elif STPYV8_LOG_LEVEL == CRITICAL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif

#endif

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

enum Loggers {
  kRootLogger = 0,
  kPythonObjectLogger,
  kContextLogger,
  kEngineLogger,
  kIsolateLogger,
  kPlatformLogger,
  kScriptLogger,
  kLockingLogger,
  kJSExceptionLogger,
  kJSStackFrameLogger,
  kJSStackTraceLogger,
  kJSObjectLogger,
  kNumLoggers
};

void useLogging();

LoggerRef getLogger(Loggers which);