#include "Script.h"
#include "JSException.h"

void CScript::Expose(const py::module& py_module) {
  // clang-format off
  py::class_<CScript, CScriptPtr>(py_module, "JSScript", "JSScript is a compiled JavaScript script.")
      .def_property_readonly("source", &CScript::GetSource,
                             "the source code")

      .def("run", &CScript::Run,
           "Execute the compiled code.");
  // clang-format on
}

std::string CScript::GetSource() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  v8::String::Utf8Value source(m_v8_isolate, Source());

  return std::string(*source, source.length());
}

py::object CScript::Run() {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  return m_engine.ExecuteScript(Script());
}
CScript::CScript(v8::Isolate* v8_isolate,
                 const CEngine& engine,
                 const v8::Persistent<v8::String>& v8_source,
                 v8::Local<v8::Script> v8_script)
    : m_v8_isolate(v8_isolate),
      m_engine(engine),
      m_v8_source(m_v8_isolate, v8_source),
      m_v8_script(m_v8_isolate, v8_script) {}

CScript::CScript(const CScript& script) : m_v8_isolate(script.m_v8_isolate), m_engine(script.m_engine) {
  auto v8_scope = v8u::openScope(m_v8_isolate);

  m_v8_source.Reset(m_v8_isolate, script.Source());
  m_v8_script.Reset(m_v8_isolate, script.Script());
}

CScript::~CScript() {
  m_v8_source.Reset();
  m_v8_script.Reset();
}
