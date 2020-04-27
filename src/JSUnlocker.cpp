#include "JSUnlocker.h"
#include "PythonThreads.h"
#include "JSIsolate.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSLockingLogger), __VA_ARGS__)

bool CJSUnlocker::IsEntered() const {
  auto result = static_cast<bool>(m_v8_unlocker.get());
  TRACE("CJSUnlocker::IsEntered {} => {}", THIS, result);
  return result;
}

void CJSUnlocker::Enter() {
  TRACE("CJSUnlocker::Enter {}", THIS);
  withAllowedPythonThreads([&]() {
    auto v8_isolate = v8u::getCurrentIsolate();
    m_isolate = CJSIsolate::FromV8(v8_isolate);
    m_v8_unlocker = std::make_unique<v8::Unlocker>(v8_isolate);
  });
}

void CJSUnlocker::Leave() {
  TRACE("CJSUnlocker::Leave {}", THIS);
  withAllowedPythonThreads([&]() {
    m_v8_unlocker.reset();
    m_isolate.reset();
  });
}
