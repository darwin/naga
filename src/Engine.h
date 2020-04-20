#pragma once

#include "Base.h"

class CEngine {
  v8::IsolateRef m_v8_isolate;

 protected:
  CScriptPtr InternalCompile(v8::Local<v8::String> v8_src, v8::Local<v8::Value> v8_name, int line, int col);

 public:
  CEngine();
  explicit CEngine(v8::IsolateRef v8_isolate);

  static void TerminateAllThreads();

  CScriptPtr Compile(const std::string& src, const std::string& name = std::string(), int line = -1, int col = -1);
  CScriptPtr CompileW(const std::wstring& src, const std::wstring& name = std::wstring(), int line = -1, int col = -1);

  static const char* GetVersion();
  static void SetStackLimit(uintptr_t stack_limit_size);

  [[nodiscard]] py::object ExecuteScript(v8::Local<v8::Script> v8_script) const;

  static void SetFlags(const std::string& flags);

  static bool IsDead();

  void Dump(std::ostream& os) const;
};