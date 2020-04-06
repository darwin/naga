#pragma once

#include "Base.h"

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
