#pragma once

#include "Base.h"

class CContext {
  py::object m_py_global;
  v8::Persistent<v8::Context> m_v8_context;

 public:
  explicit CContext(const v8::Local<v8::Context>& context);
  CContext(const CContext& context);
  explicit CContext(const py::object& py_global);

  ~CContext() { m_v8_context.Reset(); }

  [[nodiscard]] v8::Local<v8::Context> Handle() const;
  [[nodiscard]] py::object GetGlobal() const;

  py::str GetSecurityToken();
  void SetSecurityToken(const py::str& py_token) const;

  bool IsEntered() { return !m_v8_context.IsEmpty(); }
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

  static void Expose(const py::module& py_module);
};
