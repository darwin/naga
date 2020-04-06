#pragma once

#include "Base.h"
#include "Engine.h"

class CScript {
  v8::Isolate* m_v8_isolate;
  const CEngine& m_engine;

  v8::Persistent<v8::String> m_v8_source;
  v8::Persistent<v8::Script> m_v8_script;

 public:
  CScript(v8::Isolate* v8_isolate,
          const CEngine& engine,
          const v8::Persistent<v8::String>& v8_source,
          v8::Local<v8::Script> v8_script);
  CScript(const CScript& script);
  ~CScript();

  [[nodiscard]] v8::Local<v8::String> Source() const { return v8::Local<v8::String>::New(m_v8_isolate, m_v8_source); }
  [[nodiscard]] v8::Local<v8::Script> Script() const { return v8::Local<v8::Script>::New(m_v8_isolate, m_v8_script); }

  [[nodiscard]] std::string GetSource() const;
  pb::object Run();

  static void Expose(const pb::module& py_module);
};
