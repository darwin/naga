#ifndef NAGA_JSLOCKER_H_
#define NAGA_JSLOCKER_H_

#include "Base.h"

class CJSLocker {
  std::unique_ptr<v8::Locker> m_v8_locker;
  // this smart pointer is important to ensure that associated isolate outlives our locker
  CJSIsolatePtr m_isolate;

 public:
  bool IsEntered() const;
  void Enter();
  void Leave();
  static bool IsLocked();
  static bool IsActive();
};

#endif