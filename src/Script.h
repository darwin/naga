#pragma once

#include "Base.h"

class CScript {
  const CEngine& m_engine;
  v8::IsolateRef m_v8_isolate;
  v8::Global<v8::String> m_v8_source;
  v8::Global<v8::Script> m_v8_script;

 public:
  CScript(v8::IsolateRef v8_isolate,
          const CEngine& engine,
          v8::Local<v8::String> v8_source,
          v8::Local<v8::Script> v8_script);
  ~CScript();

  [[nodiscard]] v8::Local<v8::String> Source() const;
  [[nodiscard]] v8::Local<v8::Script> Script() const;

  [[nodiscard]] std::string GetSource() const;
  py::object Run();

  void Dump(std::ostream& os) const;
};
