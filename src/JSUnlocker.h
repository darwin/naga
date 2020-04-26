#ifndef NAGA_JSUNLOCKER_H_
#define NAGA_JSUNLOCKER_H_

#include "Base.h"

class CJSUnlocker {
  std::unique_ptr<v8::Unlocker> m_v8_unlocker;
  // this smart pointer is important to ensure that associated isolate outlives our unlocker
  CJSIsolatePtr m_isolate;

 public:
  bool IsEntered() const;
  void Enter();
  void Leave();
};

#endif