#ifndef NAGA_V8LOCKEDISOLATE_H_
#define NAGA_V8LOCKEDISOLATE_H_

#include "Base.h"
#include "V8ProtectedIsolate.h"

namespace v8 {

class LockedIsolatePtr {
  v8::Isolate* m_v8_isolate;
  SharedIsolateLockerPtr m_v8_shared_locker;

 public:
  explicit LockedIsolatePtr(v8::Isolate* v8_isolate, SharedIsolateLockerPtr v8_shared_locker);
  ~LockedIsolatePtr();

  SharedIsolateLockerPtr shareLocker() const { return m_v8_shared_locker; }

  v8::Isolate* operator->() { return m_v8_isolate; }
  const v8::Isolate* operator->() const { return m_v8_isolate; }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"
  // we really want implicit conversions here, so that locked isolate can be directly passed into V8 APIs
  operator v8::Isolate *() { return m_v8_isolate; }
  operator const v8::Isolate *() const { return m_v8_isolate; }
  operator ProtectedIsolatePtr() { return ProtectedIsolatePtr(m_v8_isolate); }
#pragma clang diagnostic pop
};

}  // namespace v8

#endif