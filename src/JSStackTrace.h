#ifndef NAGA_JSSTACKTRACE_H_
#define NAGA_JSSTACKTRACE_H_

#include "Base.h"
#include "V8XProtectedIsolate.h"

class JSStackTrace : public std::enable_shared_from_this<JSStackTrace> {
  v8x::ProtectedIsolatePtr m_v8_isolate;
  v8::Global<v8::StackTrace> m_v8_stack_trace;

 public:
  JSStackTrace(v8x::ProtectedIsolatePtr v8_isolate, v8::Local<v8::StackTrace> v8_stack_trace);
  JSStackTrace(const JSStackTrace& stack_trace);
  ~JSStackTrace();

  [[nodiscard]] v8::Local<v8::StackTrace> Handle() const;

  static SharedJSStackTracePtr GetCurrentStackTrace(
      v8x::LockedIsolatePtr& v8_isolate,
      int frame_limit,
      v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview);

  void Dump(std::ostream& os) const;

  // exposed API
  [[nodiscard]] int GetFrameCount() const;
  [[nodiscard]] SharedJSStackFramePtr GetFrame(int idx) const;
  [[nodiscard]] py::object Str() const;
  [[nodiscard]] SharedConstJSStackTraceIteratorPtr Iter() const;
};

#endif