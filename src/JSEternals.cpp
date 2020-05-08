#include "JSEternals.h"
#include "JSIsolate.h"
#include "Logging.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSEternalsLogger), __VA_ARGS__)

JSEternals::JSEternals(v8x::ProtectedIsolatePtr v8_protected_isolate) : m_v8_isolate(v8_protected_isolate), m_cache{} {
  TRACE("JSEternals::JSEternals {} v8_isolate={}", THIS, m_v8_isolate);
}

JSEternals::~JSEternals() {
  TRACE("JSEternals::~JSEternals {}", THIS);
}