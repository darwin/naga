#ifndef NAGA_JSCONTEXT_H_
#define NAGA_JSCONTEXT_H_

#include "Base.h"

class CJSContext : public std::enable_shared_from_this<CJSContext> {
  v8::Global<v8::Context> m_v8_context;
  // this smart pointer is important to ensure that associated isolate outlives our context
  // it should always be equal to m_v8_context->GetIsolate()
  CJSIsolatePtr m_isolate;

  // we want to keep the isolate locked between enter/leave
  v8x::SharedIsolateLockerPtr m_v8_shared_isolate_locker;
  size_t m_entered_level;

 public:
  static CJSContextPtr FromV8(v8::Local<v8::Context> v8_context);
  [[nodiscard]] v8::Local<v8::Context> ToV8() const;

  explicit CJSContext(const py::object& py_global);
  ~CJSContext();

  void Dump(std::ostream& os) const;

  [[nodiscard]] py::object GetGlobal() const;

  py::str GetSecurityToken() const;
  void SetSecurityToken(const py::str& py_token) const;

  void Enter();
  void Leave();

  static py::object GetCurrent();

  static py::object Evaluate(const std::string& src,
                             const std::string& name = std::string(),
                             int line = -1,
                             int col = -1);
  static py::object EvaluateW(const std::wstring& src,
                              const std::wstring& name = std::wstring(),
                              int line = -1,
                              int col = -1);
};

#endif