#ifndef NAGA_LOGGING_H_
#define NAGA_LOGGING_H_

#include "Base.h"

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
  kJSRegistryLogger,
  kAutoTryCatchLogger,
  kJSLandLogger,
  kPythonModuleLogger,
  kHandleScopeLogger,
  kNumLoggers
};

void useLogging();

LoggerPtr getLogger(Loggers which);
size_t giveNextMoreID();

using InceptionLevel = size_t;
void increaseCurrentInceptionLevel();
void decreaseCurrentInceptionLevel();
InceptionLevel getCurrentInceptionLevel();

using HandleScopeLevel = size_t;
void increaseCurrentHandleScopeLevel();
void decreaseCurrentHandleScopeLevel();
HandleScopeLevel getCurrentHandleScopeLevel();

class LoggerIndent {
 public:
  static size_t m_indent;

  LoggerIndent() { IncreaseIndent(); }
  ~LoggerIndent() { DecreaseIndent(); }

  static size_t GetIndent() { return m_indent; }
  static void IncreaseIndent() { m_indent++; }
  static void DecreaseIndent() { m_indent--; }
};

#define LOGGER_CONCAT_(x, y) x##y
#define LOGGER_CONCAT(x, y) LOGGER_CONCAT_(x, y)
#define LOGGER_INDENT LoggerIndent LOGGER_CONCAT(logger_indent_, __COUNTER__)
#define LOGGER_INDENT_INCREASE LoggerIndent::IncreaseIndent()
#define LOGGER_INDENT_DECREASE LoggerIndent::DecreaseIndent()

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
  if (pos == std::string::npos) {  // not multi-line
    if (s.size() < max_len) {      // not longer than max_len
      return true;
    }
  }
  return false;
}

template <class T>
std::string traceText(T&& content) {
  // trace long/multi-line content via traceMore
  auto content_str = fmt::format("{}", std::forward<T>(content));
  if (isShortString(content_str)) {
    return content_str;
  } else {
    return traceMore(content_str);
  }
}

class JSLandLogger {
  [[maybe_unused]] const char* m_name;

 public:
  explicit JSLandLogger(const char* name) : m_name(name) {
    HTRACE(kJSLandLogger, ">>> {}", m_name);
    LOGGER_INDENT_INCREASE;
    increaseCurrentInceptionLevel();
  }
  ~JSLandLogger() {
    decreaseCurrentInceptionLevel();
    LOGGER_INDENT_DECREASE;
    HTRACE(kJSLandLogger, "<<< {}", m_name);
  }
};

#endif