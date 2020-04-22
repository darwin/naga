#include "JSLocker.h"
#include "JSIsolate.h"
#include "PythonThreads.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kLockingLogger), __VA_ARGS__)

bool CJSLocker::IsEntered() {
  auto result = static_cast<bool>(m_v8_locker.get());
  TRACE("CLocker::IsEntered {} => {}", THIS, result);
  return result;
}

void CJSLocker::Enter() {
  TRACE("CLocker::Enter {}", THIS);
  withAllowedPythonThreads([&]() {
    auto v8_isolate = v8u::getCurrentIsolate();
    m_isolate = CJSIsolate::FromV8(v8_isolate);
    m_v8_locker = std::make_unique<v8::Locker>(v8_isolate);
  });
}

void CJSLocker::Leave() {
  TRACE("CLocker::Leave {}", THIS);
  withAllowedPythonThreads([&]() {
    m_v8_locker.reset();
    m_isolate.reset();
  });
}

bool CJSLocker::IsLocked() {
  auto result = v8::Locker::IsLocked(v8u::getCurrentIsolate());
  TRACE("CLocker::IsLocked => {}", result);
  return result;
}

bool CJSLocker::IsActive() {
  auto result = v8::Locker::IsActive();
  TRACE("CLocker::IsActive => {}", result);
  return result;
}
