#include "IsolateLockerHolder.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kIsolateLockingLogger), __VA_ARGS__)

ObservedLocker::ObservedLocker(v8::Isolate* v8_isolate) : v8::Locker(v8_isolate) {
  TRACE("ObservedLocker::ObservedLocker {} v8_isolate={}", THIS, P$(v8_isolate));
}

ObservedLocker::~ObservedLocker() {
  TRACE("ObservedLocker::~ObservedLocker {}", THIS);
}

IsolateLockerHolder::IsolateLockerHolder(v8::Isolate* v8_isolate) : m_v8_isolate(v8_isolate) {
  TRACE("IsolateLocker::IsolateLocker {} v8_isolate={}", THIS, P$(m_v8_isolate));
}

IsolateLockerHolder::~IsolateLockerHolder() {
  TRACE("IsolateLocker::~IsolateLocker {} v8_isolate={}", THIS, P$(m_v8_isolate));
}

v8x::SharedIsolateLockerPtr IsolateLockerHolder::CreateOrShareLocker() {
  // if locker already exists, just share it
  if (auto shared_locker = m_v8_weak_locker.lock()) {
    TRACE("IsolateLocker::CreateOrShareLocker {} v8_isolate={} sharing existing locker {}", THIS, P$(m_v8_isolate),
          (void*)shared_locker.get());
    return shared_locker;
  } else {
    // else create new locker inplace and share it
    auto v8_locker_ptr = new (m_v8_locker_storage.data()) LockerType(m_v8_isolate);
    auto new_shared_locker = v8x::SharedIsolateLockerPtr(v8_locker_ptr, DeleteInplaceLocker);
    TRACE("IsolateLocker::CreateOrShareLocker {} v8_isolate={} creating new locker {}", THIS, P$(m_v8_isolate),
          (void*)new_shared_locker.get());
    m_v8_weak_locker = new_shared_locker;
    return new_shared_locker;
  }
}

void IsolateLockerHolder::DeleteInplaceLocker(LockerType* p) {
  TRACE("IsolateLockerHolder::DeleteInplaceLocker locker={}", (void*)p);
  // just call the destructor
  // deallocation is not needed because we keep the buffer for future usage
  p->~LockerType();
}
