#include "JSStackFrame.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSStackFrameLogger), __VA_ARGS__)

JSStackFrame::JSStackFrame(v8x::ProtectedIsolatePtr v8_isolate, v8::Local<v8::StackFrame> v8_stack_frame)
    : m_v8_isolate(v8_isolate),
      m_v8_frame(v8_isolate.lock(), v8_stack_frame) {
  m_v8_frame.AnnotateStrongRetainer("Naga JSStackFrame");

  TRACE("JSStackFrame::JSStackFrame {} v8_isolate={} v8_stack_frame={}", THIS, m_v8_isolate, v8_stack_frame);
}

JSStackFrame::JSStackFrame(const JSStackFrame& stack_frame) : m_v8_isolate(stack_frame.m_v8_isolate) {
  TRACE("JSStackFrame::JSStackFrame {} stack_frame={}", THIS, stack_frame);
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  m_v8_frame.Reset(v8_isolate, stack_frame.Handle());
  m_v8_frame.AnnotateStrongRetainer("Naga JSStackFrame");
}

v8::Local<v8::StackFrame> JSStackFrame::Handle() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::StackFrame>::New(v8_isolate, m_v8_frame);
  TRACE("JSStackFrame::Handle {} => {}", THIS, result);
  return result;
}

std::string JSStackFrame::GetScriptName() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = v8x::toStdString(v8_isolate, Handle()->GetScriptName());
  TRACE("JSStackFrame::GetScriptName {} => {}", THIS, result);
  return result;
}

std::string JSStackFrame::GetFunctionName() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = v8x::toStdString(v8_isolate, Handle()->GetFunctionName());
  TRACE("JSStackFrame::GetFunctionName {} => {}", THIS, result);
  return result;
}

int JSStackFrame::GetLineNumber() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = Handle()->GetLineNumber();
  TRACE("JSStackFrame::GetLineNumber {} => {}", THIS, result);
  return result;
}

int JSStackFrame::GetColumnNumber() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = Handle()->GetColumn();
  TRACE("JSStackFrame::GetColumnNumber {} => {}", THIS, result);
  return result;
}

bool JSStackFrame::IsEval() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = Handle()->IsEval();
  TRACE("JSStackFrame::IsEval {} => {}", THIS, result);
  return result;
}

bool JSStackFrame::IsConstructor() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = Handle()->IsConstructor();
  TRACE("JSStackFrame::IsConstructor {} => {}", THIS, result);
  return result;
}

void JSStackFrame::Dump(std::ostream& os) const {
  fmt::print(os, "JSStackFrame {}", THIS);
}
