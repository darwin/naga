#pragma once

#include "Base.h"

class CEngine {
  v8::IsolateRef m_v8_isolate;

  CScriptPtr InternalCompile(v8::Local<v8::String> v8_src, v8::Local<v8::Value> v8_name, int line, int col);

 public:
  CEngine();
  explicit CEngine(v8::IsolateRef v8_isolate);

  static void SetFlags(const std::string& flags);
  static void SetStackLimit(uintptr_t stack_limit_size);

  static const char* GetVersion();
  static bool IsDead();
  static void TerminateAllThreads();

  [[nodiscard]] py::object ExecuteScript(v8::Local<v8::Script> v8_script) const;
  CScriptPtr Compile(const std::string& src, const std::string& name = std::string(), int line = -1, int col = -1);
  CScriptPtr CompileW(const std::wstring& src, const std::wstring& name = std::wstring(), int line = -1, int col = -1);

  void Dump(std::ostream& os) const;
};