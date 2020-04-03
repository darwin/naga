#pragma once

#include "Base.h"
#include "Isolate.h"

class CLocker {
  std::unique_ptr<v8::Locker> m_locker;
  CIsolatePtr m_isolate;

 public:
  CLocker() {}
  CLocker(CIsolatePtr isolate) : m_isolate(isolate) {}
  bool entered() { return m_locker.get(); }

  void enter();
  void leave();

  static bool IsLocked();
  static void Expose();
};

class CUnlocker {
  std::unique_ptr<v8::Unlocker> m_unlocker;

 public:
  bool entered() { return m_unlocker.get(); }

  void enter();
  void leave();
};
