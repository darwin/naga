#include "JSIsolateRegistry.h"
#include "JSRegistry.h"
#include "Logging.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSRegistryLogger), __VA_ARGS__)

static JSRegistry<v8::Isolate, JSIsolate> g_isolate_registry;

void registerIsolate(const v8x::ProtectedIsolatePtr v8_isolate, JSIsolate* isolate) {
  g_isolate_registry.Register(v8_isolate.giveMeRawIsolateAndTrustMe(), isolate);
}

void unregisterIsolate(const v8x::ProtectedIsolatePtr v8_isolate) {
  g_isolate_registry.Unregister(v8_isolate.giveMeRawIsolateAndTrustMe());
}

JSIsolate* lookupRegisteredIsolate(const v8::Isolate* v8_isolate) {
  return g_isolate_registry.LookupRegistered(v8_isolate);
}