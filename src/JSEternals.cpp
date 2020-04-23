#include "JSEternals.h"
#include "JSIsolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSEternalsLogger), __VA_ARGS__)

CJSEternals::CJSEternals(v8::IsolateRef v8_isolate) : m_v8_isolate(v8_isolate), m_cache{} {
  TRACE("CJSEternals::CJSEternals {} v8_isolate={}", THIS, P$(v8_isolate));
}

CJSEternals::~CJSEternals() {
  TRACE("CJSEternals::~CJSEternals {}", THIS);
}