#include "V8LockedIsolate.h"

v8::LockedIsolatePtr::LockedIsolatePtr(v8::Isolate* v8_isolate, SharedIsolateLockerPtr v8_shared_locker)
    : m_v8_isolate(v8_isolate),
      m_v8_shared_locker(v8_shared_locker) {}

v8::LockedIsolatePtr::~LockedIsolatePtr() {}
