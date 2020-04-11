#pragma once

#include "Base.h"

class CUnlocker {
  std::unique_ptr<v8::Unlocker> m_v8_unlocker;

 public:
  CUnlocker() = default;
  ~CUnlocker() = default;

  bool IsEntered();
  void Enter();
  void Leave();

  static void Expose(const py::module& py_module);
};
