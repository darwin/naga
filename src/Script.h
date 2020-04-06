#pragma once

#include "Base.h"
#include "Engine.h"

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

  static void Expose(const pb::module& m);
};
