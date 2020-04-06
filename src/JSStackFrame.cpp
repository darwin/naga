#include "JSStackFrame.h"

void CJSStackFrame::Expose(const pb::module& m) {
  // clang-format off
  pb::class_<CJSStackFrame, CJSStackFramePtr>(m, "JSStackFrame")
      .def_property_readonly("lineNum", &CJSStackFrame::GetLineNumber)
      .def_property_readonly("column", &CJSStackFrame::GetColumn)
      .def_property_readonly("scriptName", &CJSStackFrame::GetScriptName)
      .def_property_readonly("funcName", &CJSStackFrame::GetFunctionName)
      .def_property_readonly("isEval", &CJSStackFrame::IsEval)
      .def_property_readonly("isConstructor", &CJSStackFrame::IsConstructor);
  // clang-format on
}

std::string CJSStackFrame::GetScriptName() const {
  v8::HandleScope handle_scope(m_v8_isolate);

  v8::String::Utf8Value name(m_v8_isolate, Handle()->GetScriptName());

  return std::string(*name, name.length());
}

std::string CJSStackFrame::GetFunctionName() const {
  v8::HandleScope handle_scope(m_v8_isolate);

  v8::String::Utf8Value name(m_v8_isolate, Handle()->GetFunctionName());

  return std::string(*name, name.length());
}