#pragma once

#include "Base.h"

class CContext;

typedef std::shared_ptr<CContext> CContextPtr;

class CContext {
  pb::object m_py_global;
  v8::Persistent<v8::Context> m_v8_context;

 public:
  explicit CContext(const v8::Local<v8::Context>& context);
  CContext(const CContext& context);
  explicit CContext(const pb::object& py_global);

  ~CContext() { m_v8_context.Reset(); }

  [[nodiscard]] v8::Local<v8::Context> Handle() const;
  [[nodiscard]] pb::object GetGlobal() const;

  pb::str GetSecurityToken();
  void SetSecurityToken(const pb::str& py_token) const;

  bool IsEntered() { return !m_v8_context.IsEmpty(); }
  void Enter() const;
  void Leave() const;

  static bool InContext() { return v8::Isolate::GetCurrent()->InContext(); }

  static pb::object GetEntered();
  static pb::object GetCurrent();
  static pb::object GetCalling();

  static pb::object Evaluate(const std::string& src,
                             const std::string& name = std::string(),
                             int line = -1,
                             int col = -1);
  static pb::object EvaluateW(const std::wstring& src,
                              const std::wstring& name = std::wstring(),
                              int line = -1,
                              int col = -1);

  static void Expose(const pb::module& m);
};
