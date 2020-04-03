#pragma once

#include "Base.h"
#include "Exception.h"

enum IsolateDataSlot { kReserved = 0, kJSObjectTemplate = 1 };

class CIsolate;

typedef std::shared_ptr<CIsolate> CIsolatePtr;

class CIsolate {
  v8::Isolate* m_isolate;
  bool m_owner;
  void Init(bool owner);

 public:
  CIsolate();
  CIsolate(bool owner);
  CIsolate(v8::Isolate* isolate);
  ~CIsolate(void);

  v8::Isolate* GetIsolate(void);

  CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit, v8::StackTrace::StackTraceOptions options);

  static py::object GetCurrent(void);

  void Enter(void) { m_isolate->Enter(); }
  void Leave(void) { m_isolate->Exit(); }
  void Dispose(void) { m_isolate->Dispose(); }

  bool IsLocked(void) { return v8::Locker::IsLocked(m_isolate); }
};
