#ifndef NAGA_ISOLATELOCKERHOLDER_H_
#define NAGA_ISOLATELOCKERHOLDER_H_

#include "Base.h"
#include "V8XObservedLocker.h"

class IsolateLockerHolder {
  using LockerType = v8x::ObservedLocker;
  v8::Isolate* m_v8_isolate;
  v8x::WeakIsolateLockerPtr m_v8_weak_locker;
  alignas(LockerType) std::array<std::byte, sizeof(LockerType)> m_v8_locker_storage;

  static void DeleteInplaceLocker(LockerType* p);

 public:
  explicit IsolateLockerHolder(v8::Isolate* v8_isolate);
  ~IsolateLockerHolder();

  v8x::SharedIsolateLockerPtr CreateOrShareLocker();
};

#endif