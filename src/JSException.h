#ifndef NAGA_JSEXCEPTION_H_
#define NAGA_JSEXCEPTION_H_

#include "Base.h"
#include "V8XProtectedIsolate.h"

void translateException(std::exception_ptr p);

v8::Eternal<v8::Private> privateAPIForType(v8x::LockedIsolatePtr& v8_isolate);
v8::Eternal<v8::Private> privateAPIForValue(v8x::LockedIsolatePtr& v8_isolate);

class CJSException : public std::runtime_error {
  v8x::ProtectedIsolatePtr m_v8_isolate;
  PyObject* m_raw_type;

  v8::Global<v8::Value> m_v8_exception;
  v8::Global<v8::Value> m_v8_stack;
  v8::Global<v8::Message> m_v8_message;

 protected:
  CJSException(v8x::ProtectedIsolatePtr v8_isolate, const v8::TryCatch& v8_try_catch, PyObject* raw_type);

 public:
  CJSException(v8x::ProtectedIsolatePtr v8_isolate,
               const std::string& msg,
               PyObject* raw_type = PyExc_RuntimeError) noexcept;
  explicit CJSException(const std::string& msg, PyObject* raw_type = PyExc_RuntimeError) noexcept;
  CJSException(const CJSException& ex) noexcept;
  ~CJSException() noexcept;

  [[nodiscard]] PyObject* GetType() const;

  [[nodiscard]] v8::Local<v8::Value> Exception() const;
  [[nodiscard]] v8::Local<v8::Value> Stack() const;
  [[nodiscard]] v8::Local<v8::Message> Message() const;

  std::string GetName() const;
  std::string GetMessage() const;
  std::string GetScriptName() const;
  int GetLineNumber() const;
  int GetStartPosition() const;
  int GetEndPosition() const;
  int GetStartColumn() const;
  int GetEndColumn() const;
  std::string GetSourceLine() const;
  std::string GetStackTrace() const;

  [[nodiscard]] py::object Str() const;

  void PrintStackTrace(py::object py_file) const;
  static void CheckTryCatch(v8x::LockedIsolatePtr& v8_isolate, v8x::TryCatchPtr v8_try_catch);
  static void Throw(v8x::LockedIsolatePtr& v8_isolate, v8x::TryCatchPtr v8_try_catch);
};

static_assert(std::is_nothrow_copy_constructible<CJSException>::value,
              "CJSException must be nothrow copy constructible");

#endif