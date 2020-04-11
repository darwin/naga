#pragma once

#include "Base.h"

class CLocker {
  std::unique_ptr<v8::Locker> m_v8_locker;
  CIsolatePtr m_isolate;

 public:
  CLocker() = default;
  explicit CLocker(CIsolatePtr isolate);
  ~CLocker() = default;

  bool IsEntered();
  void Enter();
  void Leave();
  static bool IsLocked();
  static bool IsActive();

  static void Expose(const py::module& py_module);
};