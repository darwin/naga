#pragma once

#include "Base.h"

class CContext : public std::enable_shared_from_this<CContext> {
  py::object m_py_global;
  v8::Persistent<v8::Context> m_v8_context;
  // this smart pointer is important to ensure that associated isolate outlives our context
  // it should always be equal to m_v8_context->GetIsolate()
  CIsolatePtr m_isolate;

 public:
  static CContextPtr FromV8(v8::Local<v8::Context> v8_context);
  [[nodiscard]] v8::Local<v8::Context> ToV8() const;

  explicit CContext(const py::object& py_global);
  ~CContext();

  void Dump(std::ostream& os) const;

  [[nodiscard]] py::object GetGlobal() const;

  py::str GetSecurityToken();
  void SetSecurityToken(const py::str& py_token) const;

  bool IsEntered();
  void Enter() const;
  void Leave() const;

  static bool InContext();

  static py::object GetEntered();
  static py::object GetCurrent();
  static py::object GetCalling();

  static py::object Evaluate(const std::string& src,
                             const std::string& name = std::string(),
                             int line = -1,
                             int col = -1);
  static py::object EvaluateW(const std::wstring& src,
                              const std::wstring& name = std::wstring(),
                              int line = -1,
                              int col = -1);
};
