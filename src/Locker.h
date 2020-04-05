#pragma once

#include "Base.h"
#include "Isolate.h"

class CLocker {
  std::unique_ptr<v8::Locker> m_locker;
  CIsolatePtr m_isolate;

 public:
  CLocker() {
//    std::cerr << "LOCKER ALLOC1 " << this << "\n";
  }
  CLocker(CIsolatePtr isolate) : m_isolate(isolate) {
//    std::cerr << "LOCKER ALLOC2 " << this << "\n";
  }
  ~CLocker() {
//    std::cerr << "LOCKER DEALLOC " << this << "\n";
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
//    std::cerr << "UNLOCKER ALLOC1 " << this << "\n";
  }
  ~CUnlocker() {
//    std::cerr << "UNLOCKER DEALLOC " << this << "\n";
  }
  bool entered() { return m_unlocker.get(); }

  void enter();
  void leave();

  static void Expose(pb::module& m);
};
