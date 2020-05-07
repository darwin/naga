#include "V8XProtectedIsolate.h"
#include "V8XLockedIsolate.h"
#include "JSIsolate.h"
#include "V8XUtils.h"

namespace v8x {

ProtectedIsolatePtr::ProtectedIsolatePtr(v8::Isolate* v8_isolate) : m_v8_isolate(v8_isolate) {}

ProtectedIsolatePtr::~ProtectedIsolatePtr() {}

LockedIsolatePtr ProtectedIsolatePtr::lock() const {
  return lockIsolate(m_v8_isolate);
}

}  // namespace v8x