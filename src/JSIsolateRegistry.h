#pragma once

#include "Base.h"

using TIsolateRegistry = std::unordered_map<const v8::Isolate*, CJSIsolate*>;

void registerIsolate(const v8::Isolate* v8_isolate, CJSIsolate* isolate);
void unregisterIsolate(const v8::Isolate* v8_isolate);
CJSIsolate* lookupRegisteredIsolate(const v8::Isolate* v8_isolate);

class CJSIsolateRegistry {
  TIsolateRegistry m_registry;

 public:
  void Register(const v8::Isolate* v8_isolate, CJSIsolate* isolate);
  void Unregister(const v8::Isolate* v8_isolate);
  CJSIsolate* LookupRegistered(const v8::Isolate* v8_isolate) const;
};
