#pragma once

#include "Base.h"

class CLocker {
  std::unique_ptr<v8::Locker> m_v8_locker;
  CIsolatePtr m_isolate;

 public:
  CLocker() = default;
  explicit CLocker(CIsolatePtr isolate) : m_isolate(std::move(isolate)) {}
  ~CLocker() = default;
  bool entered() { return m_v8_locker.get(); }

  void enter();
  void leave();

  static bool IsLocked();
  static bool IsActive();

  static void Expose(const pb::module& py_module);
};

class CUnlocker {
  std::unique_ptr<v8::Unlocker> m_v8_unlocker;

 public:
  CUnlocker() = default;
  ~CUnlocker() = default;

  bool entered() { return m_v8_unlocker.get(); }
  void enter();
  void leave();

  static void Expose(const pb::module& py_module);
};
