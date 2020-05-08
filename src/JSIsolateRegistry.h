#ifndef NAGA_JSISOLATEREGISTRY_H_
#define NAGA_JSISOLATEREGISTRY_H_

#include "Base.h"

using JSIsolateRegistry = std::unordered_map<const v8::Isolate*, JSIsolate*>;

void registerIsolate(const v8x::ProtectedIsolatePtr v8_isolate, JSIsolate* isolate);
void unregisterIsolate(const v8x::ProtectedIsolatePtr v8_isolate);
JSIsolate* lookupRegisteredIsolate(const v8::Isolate* v8_isolate);

#endif