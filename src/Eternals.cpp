#include "Eternals.h"
#include "Isolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kEternalsLogger), __VA_ARGS__)

CEternals::CEternals(v8::IsolateRef v8_isolate) : m_v8_isolate(v8_isolate), m_cache{} {
  TRACE("CEternalsCache::CEternalsCache {} v8_isolate={}", THIS, P$(v8_isolate));
}

CEternals::~CEternals() {
  TRACE("CEternalsCache::~CEternalsCache {}", THIS);
}