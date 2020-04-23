#pragma once

#include "Base.h"

class CJSIsolate : public std::enable_shared_from_this<CJSIsolate> {
  v8::IsolateRef m_v8_isolate;
  std::unique_ptr<CTracer> m_tracer;
  std::unique_ptr<CJSHospital> m_hospital;
  std::unique_ptr<CJSEternals> m_eternals;

 public:
  CJSIsolate();
  ~CJSIsolate();

  CTracer& Tracer() const;
  CJSHospital& Hospital() const;
  CJSEternals& Eternals() const;

  static CIsolatePtr FromV8(const v8::IsolateRef& v8_isolate);
  [[nodiscard]] const v8::IsolateRef& ToV8() const { return m_v8_isolate; }

  CJSStackTracePtr GetCurrentStackTrace(int frame_limit,
                                        v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview) const;

  static py::object GetCurrent();

  void Enter() const;
  void Leave() const;
  void Dispose() const;

  bool IsLocked() const;
};
