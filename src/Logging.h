#pragma once

#include "Base.h"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_FUNCTION __PRETTY_FUNCTION__

#if defined(NAGA_LOG_LEVEL)
// change SPDLOG_ACTIVE_LEVEL based on NAGA_LOG_LEVEL passed from build system
#undef SPDLOG_ACTIVE_LEVEL
#if NAGA_LOG_LEVEL == TRACE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#elif NAGA_LOG_LEVEL == DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#elif NAGA_LOG_LEVEL == INFO
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#elif NAGA_LOG_LEVEL == WARN
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_WARN
#elif NAGA_LOG_LEVEL == ERROR
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#elif NAGA_LOG_LEVEL == CRITICAL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif

#endif

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

enum Loggers {
  kRootLogger = 0,
  kMoreLogger,  // this logger is used for larger dumps, e.g. source content being compiled, etc.
  kPythonObjectLogger,
  kJSContextLogger,
  kJSEngineLogger,
  kJSIsolateLogger,
  kJSPlatformLogger,
  kJSScriptLogger,
  kJSLockingLogger,
  kJSExceptionLogger,
  kJSStackFrameLogger,
  kJSStackTraceLogger,
  kJSObjectLogger,
  kTracerLogger,
  kAuxLogger,
  kPythonExposeLogger,
  kJSHospitalLogger,
  kJSEternalsLogger,
  kJSObjectFunctionImplLogger,
  kJSObjectArrayImplLogger,
  kJSObjectCLJSImplLogger,
  kJSObjectGenericImplLogger,
  kWrappingLogger,
  kJSIsolateRegistryLogger,
  kNumLoggers
};

void useLogging();

LoggerRef getLogger(Loggers which);
size_t giveNextMoreID();

class LoggerIndent {
 public:
  static size_t m_indent;

  LoggerIndent() { m_indent++; }
  ~LoggerIndent() { m_indent--; }

  static size_t GetIndent() { return m_indent; }
};

#define LOGGER_CONCAT_(x, y) x##y
#define LOGGER_CONCAT(x, y) LOGGER_CONCAT_(x, y)
#define LOGGER_INDENT LoggerIndent LOGGER_CONCAT(logger_indent_, __COUNTER__)

// for tracing from headers mainly
#define HTRACE(logger, ...) \
  LOGGER_INDENT;            \
  SPDLOG_LOGGER_TRACE(getLogger(logger), __VA_ARGS__)

template <class T>
std::string traceMore(T&& content) {
  auto number = giveNextMoreID();
  auto ref = fmt::format("MORE#{}", number);
  SPDLOG_LOGGER_TRACE(getLogger(kMoreLogger), "{}\n{}\n-----", ref, content);
  return fmt::format("<SEE MORE#{}>", number);
}

inline bool isShortString(const std::string& s, size_t max_len = 80) {
  auto pos = s.find_first_of('\n');
  if (pos == std::string::npos) {
    // not multi-line
    if (s.size() < max_len) {
      // not longer than max_len
      return true;
    }
  }
  return false;
}

template <class T>
std::string traceText(T&& content) {
  // trace long/multi-line content via traceMore
  auto content_str = fmt::format("{}", content);
  if (isShortString(content_str)) {
    return content_str;
  } else {
    return traceMore(content_str);
  }
}
