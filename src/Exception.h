#pragma once

#include "Base.h"

class CJavascriptException;

class CJavascriptStackTrace {
  v8::Isolate* m_v8_isolate;
  v8::Persistent<v8::StackTrace> m_v8_stack_trace;

 public:
  CJavascriptStackTrace(v8::Isolate* isolate, v8::Local<v8::StackTrace> st)
      : m_v8_isolate(isolate), m_v8_stack_trace(isolate, st) {}

  CJavascriptStackTrace(const CJavascriptStackTrace& st) : m_v8_isolate(st.m_v8_isolate) {
    auto v8_scope = v8u::getScope(m_v8_isolate);
    m_v8_stack_trace.Reset(m_v8_isolate, st.Handle());
  }

  [[nodiscard]] v8::Local<v8::StackTrace> Handle() const {
    return v8::Local<v8::StackTrace>::New(m_v8_isolate, m_v8_stack_trace);
  }

  [[nodiscard]] int GetFrameCount() const;
  [[nodiscard]] CJavascriptStackFramePtr GetFrame(int idx) const;

  static CJavascriptStackTracePtr GetCurrentStackTrace(
      v8::Isolate* v8_isolate,
      int frame_limit,
      v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview);

  void Dump(std::ostream& os) const;

  class FrameIterator {
    CJavascriptStackTrace* m_st;
    size_t m_idx;

   public:
    FrameIterator(CJavascriptStackTrace* st, size_t idx) : m_st(st), m_idx(idx) {}

    void increment() { m_idx++; }
    [[nodiscard]] bool equal(FrameIterator const& other) const { return m_st == other.m_st && m_idx == other.m_idx; }
    [[nodiscard]] CJavascriptStackFramePtr dereference() const { return m_st->GetFrame(m_idx); }
  };

  FrameIterator begin() { return FrameIterator(this, 0); }
  FrameIterator end() { return FrameIterator(this, GetFrameCount()); }

  [[nodiscard]] pb::object ToPythonStr() const;
};

class CJavascriptStackFrame {
  v8::Isolate* m_v8_isolate;
  v8::Persistent<v8::StackFrame> m_v8_frame;

 public:
  CJavascriptStackFrame(v8::Isolate* isolate, v8::Local<v8::StackFrame> frame)
      : m_v8_isolate(isolate), m_v8_frame(isolate, frame) {}

  CJavascriptStackFrame(const CJavascriptStackFrame& frame) : m_v8_isolate(frame.m_v8_isolate) {
    v8::HandleScope handle_scope(m_v8_isolate);

    m_v8_frame.Reset(m_v8_isolate, frame.Handle());
  }

  [[nodiscard]] v8::Local<v8::StackFrame> Handle() const {
    return v8::Local<v8::StackFrame>::New(m_v8_isolate, m_v8_frame);
  }

  [[nodiscard]] int GetLineNumber() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->GetLineNumber();
  }
  [[nodiscard]] int GetColumn() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->GetColumn();
  }
  [[nodiscard]] std::string GetScriptName() const;
  [[nodiscard]] std::string GetFunctionName() const;
  [[nodiscard]] bool IsEval() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->IsEval();
  }
  [[nodiscard]] bool IsConstructor() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->IsConstructor();
  }
};

class CJavascriptException : public std::runtime_error {
  v8::Isolate* m_v8_isolate;
  PyObject* m_raw_type;

  v8::Persistent<v8::Value> m_v8_exc;
  v8::Persistent<v8::Value> m_v8_stack;
  v8::Persistent<v8::Message> m_v8_msg;

  static std::string Extract(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch);

 protected:
  CJavascriptException(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch, PyObject* raw_type)
      : std::runtime_error(Extract(v8_isolate, v8_try_catch)), m_v8_isolate(v8_isolate), m_raw_type(raw_type) {
    auto v8_scope = v8u::getScope(m_v8_isolate);

    m_v8_exc.Reset(m_v8_isolate, v8_try_catch.Exception());

    auto stack_trace = v8_try_catch.StackTrace(v8::Isolate::GetCurrent()->GetCurrentContext());
    if (!stack_trace.IsEmpty()) {
      m_v8_stack.Reset(m_v8_isolate, stack_trace.ToLocalChecked());
    }

    m_v8_msg.Reset(m_v8_isolate, v8_try_catch.Message());
  }

 public:
  CJavascriptException(v8::Isolate* isolate, const std::string& msg, PyObject* type = nullptr) noexcept
      : std::runtime_error(msg), m_v8_isolate(isolate), m_raw_type(type) {}

  explicit CJavascriptException(const std::string& msg, PyObject* type = nullptr) noexcept
      : std::runtime_error(msg), m_v8_isolate(v8::Isolate::GetCurrent()), m_raw_type(type) {}

  CJavascriptException(const CJavascriptException& ex) noexcept
      : std::runtime_error(ex.what()), m_v8_isolate(ex.m_v8_isolate), m_raw_type(ex.m_raw_type) {
    v8::HandleScope handle_scope(m_v8_isolate);

    m_v8_exc.Reset(m_v8_isolate, ex.Exception());
    m_v8_stack.Reset(m_v8_isolate, ex.Stack());
    m_v8_msg.Reset(m_v8_isolate, ex.Message());
  }

  ~CJavascriptException() noexcept override {
    if (!m_v8_exc.IsEmpty()) {
      m_v8_exc.Reset();
    }
    if (!m_v8_msg.IsEmpty()) {
      m_v8_msg.Reset();
    }
  }

  [[nodiscard]] PyObject* GetType() const { return m_raw_type; }

  [[nodiscard]] v8::Local<v8::Value> Exception() const { return v8::Local<v8::Value>::New(m_v8_isolate, m_v8_exc); }
  [[nodiscard]] v8::Local<v8::Value> Stack() const { return v8::Local<v8::Value>::New(m_v8_isolate, m_v8_stack); }
  [[nodiscard]] v8::Local<v8::Message> Message() const { return v8::Local<v8::Message>::New(m_v8_isolate, m_v8_msg); }

  std::string GetName();
  std::string GetMessage();
  std::string GetScriptName();
  int GetLineNumber();
  int GetStartPosition();
  int GetEndPosition();
  int GetStartColumn();
  int GetEndColumn();
  std::string GetSourceLine();
  std::string GetStackTrace();

  void PrintCallStack(pb::object py_file);

  static void ThrowIf(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch);

  static void Expose(const pb::module& m);
  [[nodiscard]] pb::object ToPythonStr() const;
};

static_assert(std::is_nothrow_copy_constructible<CJavascriptException>::value,
              "CJavascriptException must be nothrow copy constructible");