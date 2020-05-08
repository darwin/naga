#ifndef NAGA_ISOLATELOCKERHOLDER_H_
#define NAGA_ISOLATELOCKERHOLDER_H_

#include "Base.h"
#include "V8XObservedLocker.h"

namespace v8x {

class IsolateLockerHolder {
  using LockerType = ObservedLocker;
  v8::Isolate* m_v8_isolate;
  WeakIsolateLockerPtr m_v8_weak_locker;
  alignas(LockerType) std::array<std::byte, sizeof(LockerType)> m_v8_locker_storage;

  static void DeleteInplaceLocker(LockerType* p);

 public:
  explicit IsolateLockerHolder(v8::Isolate* v8_isolate);
  ~IsolateLockerHolder();

  SharedIsolateLockerPtr CreateOrShareLocker();
  LockedIsolatePtr GetLockedIsolate();
};

}  // namespace v8x

#endif