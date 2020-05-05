#include "JSStackTrace.h"
#include "JSStackFrame.h"
#include "JSException.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSStackTraceLogger), __VA_ARGS__)

py::object CJSStackTrace::Str() const {
  std::stringstream ss;
  ss << *this;
  auto result = py::cast(ss.str());
  TRACE("CJSStackTrace::Str {} => {}", THIS, traceText(result));
  return result;
}

CJSStackTracePtr CJSStackTrace::GetCurrentStackTrace(v8::IsolatePtr v8_isolate,
                                                     int frame_limit,
                                                     v8::StackTrace::StackTraceOptions v8_options) {
  TRACE("CJSStackTrace::GetCurrentStackTrace v8_isolate={} frame_limit={} v8_options={:#x}", P$(v8_isolate),
        frame_limit, v8_options);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);
  auto v8_stack_trace = v8::StackTrace::CurrentStackTrace(v8_isolate, frame_limit, v8_options);
  return std::make_shared<CJSStackTrace>(v8_isolate, v8_stack_trace);
}

int CJSStackTrace::GetFrameCount() const {
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto result = Handle()->GetFrameCount();
  TRACE("CJSStackTrace::GetFrameCount {} => {}", THIS, result);
  return result;
}

CJSStackFramePtr CJSStackTrace::GetFrame(int idx) const {
  TRACE("CJSStackTrace::GetFrame {} idx={}", THIS, idx);
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto v8_try_catch = v8u::withAutoTryCatch(m_v8_isolate);
  if (idx >= Handle()->GetFrameCount()) {
    throw CJSException("index of of range", PyExc_IndexError);
  }
  auto v8_stack_frame = Handle()->GetFrame(m_v8_isolate, idx);

  auto result = std::make_shared<CJSStackFrame>(m_v8_isolate, v8_stack_frame);
  TRACE("CJSStackTrace::GetFrame {} => {}", THIS, result);
  return result;
}

CJSStackTrace::CJSStackTrace(v8::IsolatePtr v8_isolate, v8::Local<v8::StackTrace> v8_stack_trace)
    : m_v8_isolate(v8_isolate), m_v8_stack_trace(v8_isolate, v8_stack_trace) {
  m_v8_stack_trace.AnnotateStrongRetainer("Naga JSStackTrace");
  TRACE("CJSStackTrace::CJSStackTrace {} v8_isolate={} v8_stack_trace={}", THIS, P$(v8_isolate), v8_stack_trace);
}

CJSStackTrace::CJSStackTrace(const CJSStackTrace& stack_trace) : m_v8_isolate(stack_trace.m_v8_isolate) {
  TRACE("CJSStackTrace::CJSStackTrace {} stack_trace={}", THIS, stack_trace);
  auto v8_scope = v8u::withScope(m_v8_isolate);
  m_v8_stack_trace.Reset(m_v8_isolate, stack_trace.Handle());
  m_v8_stack_trace.AnnotateStrongRetainer("Naga JSStackTrace");
}

v8::Local<v8::StackTrace> CJSStackTrace::Handle() const {
  auto result = v8::Local<v8::StackTrace>::New(m_v8_isolate, m_v8_stack_trace);
  TRACE("CJSStackTrace::Handle {} => {}", THIS, result);
  return result;
}

void CJSStackTrace::Dump(std::ostream& os) const {
  auto v8_scope = v8u::withScope(m_v8_isolate);

  for (int i = 0; i < GetFrameCount(); i++) {
    auto v8_frame = GetFrame(i)->Handle();
    auto func_name = v8u::toStdString(m_v8_isolate, v8_frame->GetFunctionName());
    auto script_name = v8u::toStdString(m_v8_isolate, v8_frame->GetScriptName());

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