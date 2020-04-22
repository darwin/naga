#include "Unlocker.h"
#include "PythonThreads.h"
#include "Isolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kLockingLogger), __VA_ARGS__)

bool CUnlocker::IsEntered() {
  auto result = static_cast<bool>(m_v8_unlocker.get());
  TRACE("CUnlocker::IsEntered {} => {}", THIS, result);
  return result;
}

void CUnlocker::Enter() {
  TRACE("CUnlocker::Enter {}", THIS);
  withAllowedPythonThreads([&]() {
    auto v8_isolate = v8u::getCurrentIsolate();
    m_isolate = CIsolate::FromV8(v8_isolate);
    m_v8_unlocker = std::make_unique<v8::Unlocker>(v8_isolate);
  });
}

void CUnlocker::Leave() {
  TRACE("CUnlocker::Leave {}", THIS);
  withAllowedPythonThreads([&]() {
    m_v8_unlocker.reset();
    m_isolate.reset();
  });
}
