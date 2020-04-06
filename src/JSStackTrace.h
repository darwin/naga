#pragma once

#include "Base.h"

class CJSStackTrace {
  v8::Isolate* m_v8_isolate;
  v8::Persistent<v8::StackTrace> m_v8_stack_trace;

 public:
  CJSStackTrace(v8::Isolate* isolate, v8::Local<v8::StackTrace> st)
      : m_v8_isolate(isolate), m_v8_stack_trace(isolate, st) {}

  CJSStackTrace(const CJSStackTrace& st) : m_v8_isolate(st.m_v8_isolate) {
    auto v8_scope = v8u::getScope(m_v8_isolate);
    m_v8_stack_trace.Reset(m_v8_isolate, st.Handle());
  }

  [[nodiscard]] v8::Local<v8::StackTrace> Handle() const {
    return v8::Local<v8::StackTrace>::New(m_v8_isolate, m_v8_stack_trace);
  }

  [[nodiscard]] int GetFrameCount() const;
  [[nodiscard]] CJSStackFramePtr GetFrame(int idx) const;

  static CJSStackTracePtr GetCurrentStackTrace(
      v8::Isolate* v8_isolate,
      int frame_limit,
      v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview);

  void Dump(std::ostream& os) const;

  class FrameIterator {
    CJSStackTrace* m_st;
    size_t m_idx;

   public:
    FrameIterator(CJSStackTrace* st, size_t idx) : m_st(st), m_idx(idx) {}

    void increment() { m_idx++; }
    [[nodiscard]] bool equal(FrameIterator const& other) const { return m_st == other.m_st && m_idx == other.m_idx; }
    [[nodiscard]] CJSStackFramePtr dereference() const { return m_st->GetFrame(m_idx); }
  };

  FrameIterator begin() { return FrameIterator(this, 0); }
  FrameIterator end() { return FrameIterator(this, GetFrameCount()); }

  [[nodiscard]] pb::object ToPythonStr() const;

  static void Expose(const pb::module& m);
};