#pragma once

#include "Base.h"

class CJSStackTrace {
  v8::Isolate* m_v8_isolate;
  v8::Persistent<v8::StackTrace> m_v8_stack_trace;

 public:
  CJSStackTrace(v8::Isolate* v8_isolate, v8::Local<v8::StackTrace> v8_stack_trace);
  CJSStackTrace(const CJSStackTrace& stack_trace);

  [[nodiscard]] v8::Local<v8::StackTrace> Handle() const;
  [[nodiscard]] int GetFrameCount() const;
  [[nodiscard]] CJSStackFramePtr GetFrame(int idx) const;

  static CJSStackTracePtr GetCurrentStackTrace(
      v8::Isolate* v8_isolate,
      int frame_limit,
      v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview);

  void Dump(std::ostream& os) const;

  class FrameIterator {
    CJSStackTrace* m_stack_trace_ptr;
    size_t m_idx;

   public:
    FrameIterator(CJSStackTrace* stack_trace_ptr, size_t idx) : m_stack_trace_ptr(stack_trace_ptr), m_idx(idx) {}

    void increment() { m_idx++; }
    [[nodiscard]] bool equal(FrameIterator const& other) const {
      return m_stack_trace_ptr == other.m_stack_trace_ptr && m_idx == other.m_idx;
    }
    [[nodiscard]] CJSStackFramePtr dereference() const { return m_stack_trace_ptr->GetFrame(m_idx); }
  };

  FrameIterator begin() { return FrameIterator(this, 0); }
  FrameIterator end() { return FrameIterator(this, GetFrameCount()); }

  [[nodiscard]] pb::object ToPythonStr() const;

  static void Expose(const pb::module& m);
};