#pragma once

#include "Base.h"
#include "JSException.h"

enum IsolateDataSlot { kReserved = 0, kJSObjectTemplate = 1 };

class CIsolate {
  v8::IsolateRef m_v8_isolate;
  bool m_owner;

 public:
  CIsolate();
  explicit CIsolate(bool owner);
  explicit CIsolate(v8::IsolateRef v8_isolate);
  ~CIsolate();

  v8::IsolateRef GetIsolate();

  CJSStackTracePtr GetCurrentStackTrace(int frame_limit, v8::StackTrace::StackTraceOptions v8_options);

  static py::object GetCurrent();

  void Enter() { m_v8_isolate->Enter(); }
  void Leave() { m_v8_isolate->Exit(); }
  void Dispose() { m_v8_isolate->Dispose(); }

  bool IsLocked() { return v8::Locker::IsLocked(m_v8_isolate); }

  static void Expose(const py::module& py_module);
};
