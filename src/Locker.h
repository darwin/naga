#pragma once

#include "Base.h"
#include "Isolate.h"

class CLocker {
  std::unique_ptr<v8::Locker> m_locker;
  CIsolatePtr m_isolate;

 public:
  CLocker() {
  }
  CLocker(CIsolatePtr isolate) : m_isolate(isolate) {
  }
  ~CLocker() {
  }
  bool entered() { return m_locker.get(); }

  void enter();
  void leave();

  static bool IsLocked();
  static bool IsActive();
  static void Expose(pb::module& m);
};

class CUnlocker {
  std::unique_ptr<v8::Unlocker> m_unlocker;

 public:
  CUnlocker() {
  }
  ~CUnlocker() {
  }
  bool entered() { return m_unlocker.get(); }

  void enter();
  void leave();

  static void Expose(pb::module& m);
};
