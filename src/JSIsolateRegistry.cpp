#include "JSIsolateRegistry.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSIsolateRegistryLogger), __VA_ARGS__)

static CJSIsolateRegistry g_isolate_registry;

void registerIsolate(const v8::Isolate* v8_isolate, CJSIsolate* isolate) {
  g_isolate_registry.Register(v8_isolate, isolate);
}

void unregisterIsolate(const v8::Isolate* v8_isolate) {
  g_isolate_registry.Unregister(v8_isolate);
}

CJSIsolate* lookupRegisteredIsolate(const v8::Isolate* v8_isolate) {
  return g_isolate_registry.LookupRegistered(v8_isolate);
}

void CJSIsolateRegistry::Register(const v8::Isolate* v8_isolate, CJSIsolate* isolate) {
  TRACE("CJSIsolateRegistry::Register {} v8_isolate={} isolate={}", THIS, (void*)v8_isolate, (void*)isolate);
  assert(m_registry.find(v8_isolate) == m_registry.end());
  m_registry.insert(std::make_pair(v8_isolate, isolate));
}

void CJSIsolateRegistry::Unregister(const v8::Isolate* v8_isolate) {
  TRACE("CJSIsolateRegistry::Unregister {} v8_isolate={}", THIS, (void*)v8_isolate);
  auto it = m_registry.find(v8_isolate);
  assert(it != m_registry.end());
  m_registry.erase(it);
}

CJSIsolate* CJSIsolateRegistry::LookupRegistered(const v8::Isolate* v8_isolate) const {
  auto lookup = m_registry.find(v8_isolate);
  auto result = ([&]() {
    if (lookup != m_registry.end()) {
      return lookup->second;
    } else {
      return static_cast<CJSIsolate*>(nullptr);
    }
  })();
  TRACE("CJSIsolateRegistry::LookupRegistered {} v8_isolate={} => {}", THIS, (void*)v8_isolate, (void*)result);
  return result;
}