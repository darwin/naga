#ifndef NAGA_V8PROTECTEDISOLATE_H_
#define NAGA_V8PROTECTEDISOLATE_H_

#include "Base.h"

namespace v8 {

class LockedIsolatePtr;

class ProtectedIsolatePtr {
  v8::Isolate* m_v8_isolate;

 public:
  explicit ProtectedIsolatePtr(v8::Isolate* v8_isolate);
  ~ProtectedIsolatePtr();

  LockedIsolatePtr lock() const;

  // use this only when you are sure what your are doing
  v8::Isolate* giveMeRawIsolateAndTrustMe() const { return m_v8_isolate; }
};

}  // namespace v8

#endif