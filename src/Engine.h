#pragma once

#include "Base.h"

class CScript;

typedef std::shared_ptr<CScript> CScriptPtr;

class CEngine {
  v8::Isolate* m_v8_isolate;

 protected:
  CScriptPtr InternalCompile(v8::Local<v8::String> v8_src, v8::Local<v8::Value> v8_name, int line, int col);
  static void TerminateAllThreads();

 public:
  explicit CEngine(v8::Isolate* v8_isolate = nullptr);

  CScriptPtr Compile(const std::string& src, const std::string& name = std::string(), int line = -1, int col = -1);
  CScriptPtr CompileW(const std::wstring& src, const std::wstring& name = std::wstring(), int line = -1, int col = -1);

 public:
  static const char* GetVersion();
  static void SetStackLimit(uintptr_t stack_limit_size);

  [[nodiscard]] pb::object ExecuteScript(v8::Local<v8::Script> v8_script) const;

  static void SetFlags(const std::string& flags);

  static bool IsDead();

  static void Expose(const pb::module& m);
};

class CScript {
  v8::Isolate* m_isolate;
  const CEngine& m_engine;

  v8::Persistent<v8::String> m_source;
  v8::Persistent<v8::Script> m_script;

 public:
  CScript(v8::Isolate* isolate,
          const CEngine& engine,
          const v8::Persistent<v8::String>& source,
          v8::Local<v8::Script> script)
      : m_isolate(isolate), m_engine(engine), m_source(m_isolate, source), m_script(m_isolate, script) {}

  CScript(const CScript& script) : m_isolate(script.m_isolate), m_engine(script.m_engine) {
    v8::HandleScope handle_scope(m_isolate);

    m_source.Reset(m_isolate, script.Source());
    m_script.Reset(m_isolate, script.Script());
  }

  ~CScript() {
    m_source.Reset();
    m_script.Reset();
  }

  [[nodiscard]] v8::Local<v8::String> Source() const { return v8::Local<v8::String>::New(m_isolate, m_source); }
  [[nodiscard]] v8::Local<v8::Script> Script() const { return v8::Local<v8::Script>::New(m_isolate, m_script); }

  [[nodiscard]] std::string GetSource() const;

  pb::object Run();
};
