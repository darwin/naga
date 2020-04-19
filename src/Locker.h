#pragma once

#include "Base.h"

class CLocker {
  std::unique_ptr<v8::Locker> m_v8_locker;
  // this smart pointer is important to ensure that associated isolate outlives our locker
  CIsolatePtr m_isolate;

 public:
  CLocker() = default;
  ~CLocker() = default;

  bool IsEntered();
  void Enter();
  void Leave();
  static bool IsLocked();
  static bool IsActive();

  static void Expose(py::module py_module);
};