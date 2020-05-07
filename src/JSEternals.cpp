#include "JSEternals.h"
#include "JSIsolate.h"
#include "Logging.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSEternalsLogger), __VA_ARGS__)

CJSEternals::CJSEternals(v8x::ProtectedIsolatePtr v8_protected_isolate)
    : m_v8_isolate(v8_protected_isolate),
      m_cache{} {
  TRACE("CJSEternals::CJSEternals {} v8_isolate={}", THIS, m_v8_isolate);
}

CJSEternals::~CJSEternals() {
  TRACE("CJSEternals::~CJSEternals {}", THIS);
}