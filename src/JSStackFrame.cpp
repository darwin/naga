#include "JSStackFrame.h"

#define TRACE(...) RAII_LOGGER_INDENT; SPDLOG_LOGGER_TRACE(getLogger(kJSStackFrameLogger), __VA_ARGS__)

void CJSStackFrame::Expose(const py::module& py_module) {
  TRACE("CJSStackFrame::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CJSStackFrame, CJSStackFramePtr>(py_module, "JSStackFrame")
      .def_property_readonly("lineNum", &CJSStackFrame::GetLineNumber)
      .def_property_readonly("column", &CJSStackFrame::GetColumn)
      .def_property_readonly("scriptName", &CJSStackFrame::GetScriptName)
      .def_property_readonly("funcName", &CJSStackFrame::GetFunctionName)
      .def_property_readonly("isEval", &CJSStackFrame::IsEval)
      .def_property_readonly("isConstructor", &CJSStackFrame::IsConstructor);
  // clang-format on
}

std::string CJSStackFrame::GetScriptName() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  v8::String::Utf8Value name(m_v8_isolate, Handle()->GetScriptName());
  auto result = std::string(*name, name.length());
  TRACE("CJSStackFrame::GetScriptName {} => {}", THIS, result);
  return result;
}

std::string CJSStackFrame::GetFunctionName() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  v8::String::Utf8Value name(m_v8_isolate, Handle()->GetFunctionName());
  auto result = std::string(*name, name.length());
  TRACE("CJSStackFrame::GetFunctionName {} => {}", THIS, result);
  return result;
}

CJSStackFrame::CJSStackFrame(const v8::IsolateRef& v8_isolate, v8::Local<v8::StackFrame> v8_stack_frame)
    : m_v8_isolate(v8_isolate), m_v8_frame(v8_isolate, v8_stack_frame) {
  TRACE("CJSStackFrame::CJSStackFrame {} v8_isolate={} v8_stack_frame={}", THIS, isolateref_printer{m_v8_isolate},
        v8_stack_frame);
}

CJSStackFrame::CJSStackFrame(const CJSStackFrame& stack_frame) : m_v8_isolate(stack_frame.m_v8_isolate) {
  TRACE("CJSStackFrame::CJSStackFrame {} stack_frame={}", THIS, stack_frame);
  auto v8_scope = v8u::openScope(m_v8_isolate);
  m_v8_frame.Reset(m_v8_isolate, stack_frame.Handle());
}

v8::Local<v8::StackFrame> CJSStackFrame::Handle() const {
  auto result = v8::Local<v8::StackFrame>::New(m_v8_isolate, m_v8_frame);
  TRACE("CJSStackFrame::Handle {} => {}", THIS, result);
  return result;
}

int CJSStackFrame::GetLineNumber() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  auto result = Handle()->GetLineNumber();
  TRACE("CJSStackFrame::GetLineNumber {} => {}", THIS, result);
  return result;
}

int CJSStackFrame::GetColumn() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  auto result = Handle()->GetColumn();
  TRACE("CJSStackFrame::GetColumn {} => {}", THIS, result);
  return result;
}

bool CJSStackFrame::IsEval() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  auto result = Handle()->IsEval();
  TRACE("CJSStackFrame::IsEval {} => {}", THIS, result);
  return result;
}

bool CJSStackFrame::IsConstructor() const {
  auto v8_scope = v8u::openScope(m_v8_isolate);
  auto result = Handle()->IsConstructor();
  TRACE("CJSStackFrame::IsConstructor {} => {}", THIS, result);
  return result;
}

void CJSStackFrame::Dump(std::ostream& os) const {
  fmt::print(os, "CJSStackFrame {}", THIS);
}
