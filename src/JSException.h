#pragma once

#include "Base.h"

class CJSException : public std::runtime_error {
  v8::Isolate* m_v8_isolate;
  PyObject* m_raw_type;

  v8::Persistent<v8::Value> m_v8_exc;
  v8::Persistent<v8::Value> m_v8_stack;
  v8::Persistent<v8::Message> m_v8_msg;

  static std::string Extract(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch);

 protected:
  CJSException(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch, PyObject* raw_type)
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
  CJSException(v8::Isolate* isolate, const std::string& msg, PyObject* type = nullptr) noexcept
      : std::runtime_error(msg), m_v8_isolate(isolate), m_raw_type(type) {}

  explicit CJSException(const std::string& msg, PyObject* type = nullptr) noexcept
      : std::runtime_error(msg), m_v8_isolate(v8::Isolate::GetCurrent()), m_raw_type(type) {}

  CJSException(const CJSException& ex) noexcept
      : std::runtime_error(ex.what()), m_v8_isolate(ex.m_v8_isolate), m_raw_type(ex.m_raw_type) {
    v8::HandleScope handle_scope(m_v8_isolate);

    m_v8_exc.Reset(m_v8_isolate, ex.Exception());
    m_v8_stack.Reset(m_v8_isolate, ex.Stack());
    m_v8_msg.Reset(m_v8_isolate, ex.Message());
  }

  ~CJSException() noexcept override {
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

static_assert(std::is_nothrow_copy_constructible<CJSException>::value,
              "CJSException must be nothrow copy constructible");