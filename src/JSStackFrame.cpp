#include "JSStackFrame.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSStackFrameLogger), __VA_ARGS__)

CJSStackFrame::CJSStackFrame(v8::ProtectedIsolatePtr v8_isolate, v8::Local<v8::StackFrame> v8_stack_frame)
    : m_v8_isolate(v8_isolate),
      m_v8_frame(v8_isolate.lock(), v8_stack_frame) {
  m_v8_frame.AnnotateStrongRetainer("Naga JSStackFrame");

  TRACE("CJSStackFrame::CJSStackFrame {} v8_isolate={} v8_stack_frame={}", THIS, m_v8_isolate, v8_stack_frame);
}

CJSStackFrame::CJSStackFrame(const CJSStackFrame& stack_frame) : m_v8_isolate(stack_frame.m_v8_isolate) {
  TRACE("CJSStackFrame::CJSStackFrame {} stack_frame={}", THIS, stack_frame);
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8u::withScope(v8_isolate);
  m_v8_frame.Reset(v8_isolate, stack_frame.Handle());
  m_v8_frame.AnnotateStrongRetainer("Naga JSStackFrame");
}

v8::Local<v8::StackFrame> CJSStackFrame::Handle() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::StackFrame>::New(v8_isolate, m_v8_frame);
  TRACE("CJSStackFrame::Handle {} => {}", THIS, result);
  return result;
}

std::string CJSStackFrame::GetScriptName() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = v8u::toStdString(v8_isolate, Handle()->GetScriptName());
  TRACE("CJSStackFrame::GetScriptName {} => {}", THIS, result);
  return result;
}

std::string CJSStackFrame::GetFunctionName() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = v8u::toStdString(v8_isolate, Handle()->GetFunctionName());
  TRACE("CJSStackFrame::GetFunctionName {} => {}", THIS, result);
  return result;
}

int CJSStackFrame::GetLineNumber() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = Handle()->GetLineNumber();
  TRACE("CJSStackFrame::GetLineNumber {} => {}", THIS, result);
  return result;
}

int CJSStackFrame::GetColumnNumber() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = Handle()->GetColumn();
  TRACE("CJSStackFrame::GetColumnNumber {} => {}", THIS, result);
  return result;
}

bool CJSStackFrame::IsEval() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = Handle()->IsEval();
  TRACE("CJSStackFrame::IsEval {} => {}", THIS, result);
  return result;
}

bool CJSStackFrame::IsConstructor() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = Handle()->IsConstructor();
  TRACE("CJSStackFrame::IsConstructor {} => {}", THIS, result);
  return result;
}

void CJSStackFrame::Dump(std::ostream& os) const {
  fmt::print(os, "CJSStackFrame {}", THIS);
}
