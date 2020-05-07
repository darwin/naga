#include "V8ProtectedIsolate.h"
#include "V8LockedIsolate.h"
#include "JSIsolate.h"
#include "V8Utils.h"

namespace v8 {

ProtectedIsolatePtr::ProtectedIsolatePtr(Isolate* v8_isolate) : m_v8_isolate(v8_isolate) {}

ProtectedIsolatePtr::~ProtectedIsolatePtr() {}

LockedIsolatePtr ProtectedIsolatePtr::lock() const {
  return v8u::lockIsolate(m_v8_isolate);
}

};  // namespace v8
