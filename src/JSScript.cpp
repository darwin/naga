#include "JSScript.h"
#include "JSEngine.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSScriptLogger), __VA_ARGS__)

CJSScript::CJSScript(v8x::ProtectedIsolatePtr v8_protected_isolate,
                     const CJSEngine& engine,
                     v8::Local<v8::String> v8_source,
                     v8::Local<v8::Script> v8_script)
    : m_engine(engine),
      m_v8_isolate(v8_protected_isolate) {
  auto v8_isolate = m_v8_isolate.lock();

  m_v8_source.Reset(v8_isolate, v8_source);
  m_v8_source.AnnotateStrongRetainer("Naga JSScript.m_v8_source");

  m_v8_script.Reset(v8_isolate, v8_script);
  m_v8_script.AnnotateStrongRetainer("Naga JSScript.m_v8_script");

  TRACE("CJSScript::CJSScript {} v8_isolate={} engine={} v8_source={} v8_script={}", THIS, P$(v8_isolate), engine,
        traceMore(v8_source), v8_script);
}

CJSScript::~CJSScript() {
  TRACE("CJSScript::~CJSScript {}", THIS);
  m_v8_source.Reset();
  m_v8_script.Reset();
}

v8::Local<v8::String> CJSScript::Source() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::String>::New(v8_isolate, m_v8_source);
  TRACE("CJSScript::Source {} => {}", THIS, traceText(result));
  return result;
}

v8::Local<v8::Script> CJSScript::Script() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::Script>::New(v8_isolate, m_v8_script);
  TRACE("CJSScript::Script {} => {}", THIS, result);
  return result;
}

std::string CJSScript::GetSource() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  v8::String::Utf8Value source(v8_isolate, Source());
  auto result = std::string(*source, source.length());
  TRACE("CJSScript::GetSource {} => {}", THIS, traceText(result));
  return result;
}

py::object CJSScript::Run() const {
  TRACE("CJSScript::Run {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = m_engine.ExecuteScript(Script());
  TRACE("CJSScript::Run {} => {}", THIS, result);
  return result;
}

void CJSScript::Dump(std::ostream& os) const {
  fmt::print(os, "CJSScript {}", THIS);
}
