#include "JSStackFrame.h"

void CJSStackFrame::Expose(const py::module& m) {
  // clang-format off
  py::class_<CJSStackFrame, CJSStackFramePtr>(m, "JSStackFrame")
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

CJSStackFrame::CJSStackFrame(v8::Isolate* v8_isolate, v8::Local<v8::StackFrame> v8_stack_frame)
    : m_v8_isolate(v8_isolate), m_v8_frame(v8_isolate, v8_stack_frame) {}

CJSStackFrame::CJSStackFrame(const CJSStackFrame& stack_frame) : m_v8_isolate(stack_frame.m_v8_isolate) {
  v8::HandleScope handle_scope(m_v8_isolate);

  m_v8_frame.Reset(m_v8_isolate, stack_frame.Handle());
}

v8::Local<v8::StackFrame> CJSStackFrame::Handle() const {
  return v8::Local<v8::StackFrame>::New(m_v8_isolate, m_v8_frame);
}

int CJSStackFrame::GetLineNumber() const {
  v8::HandleScope handle_scope(m_v8_isolate);
  return Handle()->GetLineNumber();
}

int CJSStackFrame::GetColumn() const {
  v8::HandleScope handle_scope(m_v8_isolate);
  return Handle()->GetColumn();
}

bool CJSStackFrame::IsEval() const {
  v8::HandleScope handle_scope(m_v8_isolate);
  return Handle()->IsEval();
}

bool CJSStackFrame::IsConstructor() const {
  v8::HandleScope handle_scope(m_v8_isolate);
  return Handle()->IsConstructor();
}
