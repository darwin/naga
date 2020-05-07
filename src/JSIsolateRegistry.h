#ifndef NAGA_JSISOLATEREGISTRY_H_
#define NAGA_JSISOLATEREGISTRY_H_

#include "Base.h"

using TIsolateRegistry = std::unordered_map<const v8::Isolate*, CJSIsolate*>;

void registerIsolate(const v8x::ProtectedIsolatePtr v8_isolate, CJSIsolate* isolate);
void unregisterIsolate(const v8x::ProtectedIsolatePtr v8_isolate);
CJSIsolate* lookupRegisteredIsolate(const v8::Isolate* v8_isolate);

#endif