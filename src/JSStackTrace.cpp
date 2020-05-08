#include "JSStackTrace.h"
#include "JSStackFrame.h"
#include "JSException.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSStackTraceLogger), __VA_ARGS__)

py::object JSStackTrace::Str() const {
  std::stringstream ss;
  ss << *this;
  auto result = py::cast(ss.str());
  TRACE("JSStackTrace::Str {} => {}", THIS, traceText(result));
  return result;
}

SharedJSStackTracePtr JSStackTrace::GetCurrentStackTrace(v8x::LockedIsolatePtr& v8_isolate,
                                                         int frame_limit,
                                                         v8::StackTrace::StackTraceOptions v8_options) {
  TRACE("JSStackTrace::GetCurrentStackTrace v8_isolate={} frame_limit={} v8_options={:#x}", P$(v8_isolate), frame_limit,
        v8_options);
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_stack_trace = v8::StackTrace::CurrentStackTrace(v8_isolate, frame_limit, v8_options);
  return std::make_shared<JSStackTrace>(v8_isolate, v8_stack_trace);
}

int JSStackTrace::GetFrameCount() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = Handle()->GetFrameCount();
  TRACE("JSStackTrace::GetFrameCount {} => {}", THIS, result);
  return result;
}

SharedJSStackFramePtr JSStackTrace::GetFrame(int idx) const {
  TRACE("JSStackTrace::GetFrame {} idx={}", THIS, idx);
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  if (idx >= Handle()->GetFrameCount()) {
    throw JSException("index of of range", PyExc_IndexError);
  }
  auto v8_stack_frame = Handle()->GetFrame(v8_isolate, idx);

  auto result = std::make_shared<JSStackFrame>(v8_isolate, v8_stack_frame);
  TRACE("JSStackTrace::GetFrame {} => {}", THIS, result);
  return result;
}

JSStackTrace::JSStackTrace(v8x::ProtectedIsolatePtr v8_isolate, v8::Local<v8::StackTrace> v8_stack_trace)
    : m_v8_isolate(v8_isolate),
      m_v8_stack_trace(v8_isolate.lock(), v8_stack_trace) {
  m_v8_stack_trace.AnnotateStrongRetainer("Naga JSStackTrace");
  TRACE("JSStackTrace::JSStackTrace {} v8_isolate={} v8_stack_trace={}", THIS, v8_isolate, v8_stack_trace);
}

JSStackTrace::JSStackTrace(const JSStackTrace& stack_trace) : m_v8_isolate(stack_trace.m_v8_isolate) {
  TRACE("JSStackTrace::JSStackTrace {} stack_trace={}", THIS, stack_trace);
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  m_v8_stack_trace.Reset(v8_isolate, stack_trace.Handle());
  m_v8_stack_trace.AnnotateStrongRetainer("Naga JSStackTrace");
}

v8::Local<v8::StackTrace> JSStackTrace::Handle() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::StackTrace>::New(v8_isolate, m_v8_stack_trace);
  TRACE("JSStackTrace::Handle {} => {}", THIS, result);
  return result;
}

void JSStackTrace::Dump(std::ostream& os) const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);

  for (int i = 0; i < GetFrameCount(); i++) {
    auto v8_frame = GetFrame(i)->Handle();
    auto func_name = v8x::toStdString(v8_isolate, v8_frame->GetFunctionName());
    auto script_name = v8x::toStdString(v8_isolate, v8_frame->GetScriptName());

    os << "\tat ";

    if (func_name.size()) {
      os << func_name  //
         << " (";      //
    }
    if (v8_frame->IsEval()) {
      os << "(eval)";
    } else {
      os << script_name                //
         << ":"                        //
         << v8_frame->GetLineNumber()  //
         << ":"                        //
         << v8_frame->GetColumn();     //
    }
    if (func_name.size()) {
      os << ")";
    }

    os << std::endl;
  }
}