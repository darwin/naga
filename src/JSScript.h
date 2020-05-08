#ifndef NAGA_JSSCRIPT_H_
#define NAGA_JSSCRIPT_H_

#include "Base.h"
#include "V8XProtectedIsolate.h"

class JSScript {
  const JSEngine& m_engine;
  v8x::ProtectedIsolatePtr m_v8_isolate;
  v8::Global<v8::String> m_v8_source;
  v8::Global<v8::Script> m_v8_script;

 public:
  JSScript(v8x::ProtectedIsolatePtr v8_isolate,
           const JSEngine& engine,
           v8::Local<v8::String> v8_source,
           v8::Local<v8::Script> v8_script);
  ~JSScript();

  [[nodiscard]] v8::Local<v8::String> Source() const;
  [[nodiscard]] v8::Local<v8::Script> Script() const;

  [[nodiscard]] std::string GetSource() const;
  py::object Run() const;

  void Dump(std::ostream& os) const;
};

#endif