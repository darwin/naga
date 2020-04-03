#pragma once

#include "Base.h"

class CContext;
class CIsolate;

typedef std::shared_ptr<CContext> CContextPtr;

class CContext {
  py::object m_global;
  v8::Persistent<v8::Context> m_context;

 public:
  explicit CContext(v8::Local<v8::Context> context);
  CContext(const CContext& context);
  explicit CContext(const py::object& global);

  ~CContext() { m_context.Reset(); }

  v8::Local<v8::Context> Handle() const { return v8::Local<v8::Context>::New(v8::Isolate::GetCurrent(), m_context); }

  py::object GetGlobal() const;

  py::str GetSecurityToken();
  void SetSecurityToken(const py::str& token) const;

  bool IsEntered() { return !m_context.IsEmpty(); }
  void Enter() const {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
    Handle()->Enter();
  }
  void Leave() const {
    v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
    Handle()->Exit();
  }

  py::object Evaluate(const std::string& src, const std::string& name = std::string(), int line = -1, int col = -1);
  py::object EvaluateW(const std::wstring& src, const std::wstring& name = std::wstring(), int line = -1, int col = -1);

  static py::object GetEntered();
  static py::object GetCurrent();
  static py::object GetCalling();
  static bool InContext() { return v8::Isolate::GetCurrent()->InContext(); }

  static void Expose();
};
