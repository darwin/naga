#include "Script.h"
#include "JSException.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kScriptLogger), __VA_ARGS__)

void CScript::Expose(const py::module& py_module) {
  TRACE("CScript::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CScript, CScriptPtr>(py_module, "JSScript", "JSScript is a compiled JavaScript script.")
      .def_property_readonly("source", &CScript::GetSource,
                             "the source code")

      .def("run", &CScript::Run,
           "Execute the compiled code.");
  // clang-format on
}

CScript::CScript(v8::IsolateRef v8_isolate,
                 const CEngine& engine,
                 v8::Local<v8::String> v8_source,
                 v8::Local<v8::Script> v8_script)
    : m_engine(engine),
      m_v8_isolate(std::move(v8_isolate)),
      m_v8_source(m_v8_isolate, v8_source),
      m_v8_script(m_v8_isolate, v8_script) {
  TRACE("CScript::CScript {} v8_isolate={} engine={} v8_source={} v8_script={}", THIS, isolateref_printer{v8_isolate},
        engine, v8_source, v8_script);
}

CScript::CScript(const CScript& script) : m_engine(script.m_engine), m_v8_isolate(script.m_v8_isolate) {
  TRACE("CScript::CScript {} script={}", THIS, script);
  auto v8_scope = v8u::openScope(m_v8_isolate);

  m_v8_source.Reset(m_v8_isolate, script.Source());
  m_v8_script.Reset(m_v8_isolate, script.Script());
}

CScript::~CScript() {
  TRACE("CScript::~CScript {}", THIS);
  m_v8_source.Reset();
  m_v8_script.Reset();
}

v8::Local<v8::String> CScript::Source() const {
  auto result = v8::Local<v8::String>::New(m_v8_isolate, m_v8_source);
  TRACE("CScript::Source {} => {}", THIS, result);
  return result;
}

v8::Local<v8::Script> CScript::Script() const {
  auto result = v8::Local<v8::Script>::New(m_v8_isolate, m_v8_script);
  TRACE("CScript::Script {} => {}", THIS, result);
  return result;
}

std::string CScript::GetSource() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  v8::String::Utf8Value source(m_v8_isolate, Source());
  auto result = std::string(*source, source.length());
  TRACE("CScript::GetSource {} => {}", THIS, result);
  return result;
}

py::object CScript::Run() {
  TRACE("CScript::Run {}", THIS);
  auto v8_scope = v8u::openScope(m_v8_isolate);
  auto result = m_engine.ExecuteScript(Script());
  TRACE("CScript::Run {} => {}", THIS, result);
  return result;
}

void CScript::Dump(std::ostream& os) const {
  fmt::print(os, "CScript {}", THIS);
}
