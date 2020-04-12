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

  CJSStackTracePtr GetCurrentStackTrace(int frame_limit,
                                        v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview);

  static py::object GetCurrent();

  void Enter();
  void Leave();
  void Dispose();

  bool IsLocked();

  static void Expose(const py::module& py_module);
};
