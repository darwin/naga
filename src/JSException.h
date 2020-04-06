#pragma once

#include "Base.h"

class CJSException : public std::runtime_error {
  v8::Isolate* m_v8_isolate;
  PyObject* m_raw_type;

  v8::Persistent<v8::Value> m_v8_exception;
  v8::Persistent<v8::Value> m_v8_stack;
  v8::Persistent<v8::Message> m_v8_message;

  static std::string Extract(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch);

 protected:
  CJSException(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch, PyObject* raw_type);

 public:
  CJSException(v8::Isolate* v8_isolate, const std::string& msg, PyObject* raw_type = nullptr) noexcept;
  explicit CJSException(const std::string& msg, PyObject* raw_type = nullptr) noexcept;
  CJSException(const CJSException& ex) noexcept;
  ~CJSException() noexcept override;

  [[nodiscard]] PyObject* GetType() const;

  [[nodiscard]] v8::Local<v8::Value> Exception() const;
  [[nodiscard]] v8::Local<v8::Value> Stack() const;
  [[nodiscard]] v8::Local<v8::Message> Message() const;

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

  [[nodiscard]] pb::object ToPythonStr() const;

  void PrintCallStack(pb::object py_file);
  static void ThrowIf(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch);

  static void Expose(const pb::module& py_module);
};

static_assert(std::is_nothrow_copy_constructible<CJSException>::value,
              "CJSException must be nothrow copy constructible");