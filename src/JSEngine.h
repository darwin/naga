#ifndef NAGA_JSENGINE_H_
#define NAGA_JSENGINE_H_

#include "Base.h"
#include "V8XProtectedIsolate.h"

class JSEngine {
  v8x::ProtectedIsolatePtr m_v8_isolate;

  SharedJSScriptPtr InternalCompile(v8x::LockedIsolatePtr& v8_isolate,
                                    v8::Local<v8::String> v8_src,
                                    v8::Local<v8::Value> v8_name,
                                    int line,
                                    int col) const;

 public:
  JSEngine();
  explicit JSEngine(v8x::ProtectedIsolatePtr v8_isolate);

  static void SetFlags(const std::string& flags);
  static void SetStackLimit(uintptr_t stack_limit_size);

  static const char* GetVersion();
  static bool IsDead();
  static void TerminateAllThreads();

  [[nodiscard]] py::object ExecuteScript(v8::Local<v8::Script> v8_script) const;
  SharedJSScriptPtr Compile(const std::string& src,
                            const std::string& name = std::string(),
                            int line = -1,
                            int col = -1) const;
  SharedJSScriptPtr CompileW(const std::wstring& src,
                             const std::wstring& name = std::wstring(),
                             int line = -1,
                             int col = -1) const;

  void Dump(std::ostream& os) const;
};

#endif