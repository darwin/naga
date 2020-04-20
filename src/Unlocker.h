#pragma once

#include "Base.h"

class CUnlocker {
  std::unique_ptr<v8::Unlocker> m_v8_unlocker;
  // this smart pointer is important to ensure that associated isolate outlives our unlocker
  CIsolatePtr m_isolate;

 public:
  bool IsEntered();
  void Enter();
  void Leave();
};
