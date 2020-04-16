#pragma once

#include "Base.h"
#include "JSException.h"
#include "Tracer.h"

enum IsolateDataSlot { kReserved = 0, kJSObjectTemplate = 1 };

class CIsolate : public std::enable_shared_from_this<CIsolate> {
  v8::IsolateRef m_v8_isolate;
  CTracer* m_tracer;

 public:
  CIsolate();
  ~CIsolate();

  CTracer* Tracer();

  static CIsolatePtr FromV8(const v8::IsolateRef& v8_isolate);
  [[nodiscard]] const v8::IsolateRef& ToV8() { return m_v8_isolate; }

  CJSStackTracePtr GetCurrentStackTrace(int frame_limit,
                                        v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview);

  static py::object GetCurrent();

  void Enter();
  void Leave();
  void Dispose();

  bool IsLocked();

  static void Expose(const py::module& py_module);
};
