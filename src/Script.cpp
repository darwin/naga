#include "Script.h"
#include "Exception.h"

void CScript::Expose(const pb::module& m) {
  // clang-format off
  pb::class_<CScript, CScriptPtr>(m, "JSScript", "JSScript is a compiled JavaScript script.")
      .def_property_readonly("source", &CScript::GetSource,
                             "the source code")

      .def("run", &CScript::Run,
           "Execute the compiled code.");
  // clang-format on
}

std::string CScript::GetSource() const {
  v8::HandleScope handle_scope(m_isolate);

  v8::String::Utf8Value source(m_isolate, Source());

  return std::string(*source, source.length());
}

pb::object CScript::Run() {
  auto v8_scope = v8u::getScope(m_isolate);
  return m_engine.ExecuteScript(Script());
}
