#include "JSStackTrace.h"
#include "JSStackFrame.h"
#include "JSException.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSStackTraceLogger), __VA_ARGS__)

py::object CJSStackTrace::ToPythonStr() const {
  std::stringstream ss;
  ss << *this;
  auto result = py::cast(ss.str());
  TRACE("CJSStackTrace::ToPythonStr {} => {}", THIS, result);
  return result;
}

CJSStackTracePtr CJSStackTrace::GetCurrentStackTrace(v8::IsolateRef v8_isolate,
                                                     int frame_limit,
                                                     v8::StackTrace::StackTraceOptions v8_options) {
  TRACE("CJSStackTrace::GetCurrentStackTrace v8_isolate={} frame_limit={} v8_options={:#x}",
        isolateref_printer{v8_isolate}, frame_limit, v8_options);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_stack_trace = v8::StackTrace::CurrentStackTrace(v8_isolate, frame_limit, v8_options);
  if (v8_stack_trace.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

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
  auto v8_try_catch = v8u::withTryCatch(m_v8_isolate);
  if (idx >= Handle()->GetFrameCount()) {
    throw CJSException("index of of range", PyExc_IndexError);
  }
  auto v8_stack_frame = Handle()->GetFrame(m_v8_isolate, idx);

  if (v8_stack_frame.IsEmpty()) {
    CJSException::ThrowIf(m_v8_isolate, v8_try_catch);
  }

  auto result = std::make_shared<CJSStackFrame>(m_v8_isolate, v8_stack_frame);
  TRACE("CJSStackTrace::GetFrame {} => {}", THIS, result);
  return result;
}

CJSStackTrace::CJSStackTrace(const v8::IsolateRef& v8_isolate, v8::Local<v8::StackTrace> v8_stack_trace)
    : m_v8_isolate(v8_isolate), m_v8_stack_trace(v8_isolate, v8_stack_trace) {
  TRACE("CJSStackTrace::CJSStackTrace {} v8_isolate={} v8_stack_trace={}", THIS, isolateref_printer{v8_isolate},
        v8_stack_trace);
}

CJSStackTrace::CJSStackTrace(const CJSStackTrace& stack_trace) : m_v8_isolate(stack_trace.m_v8_isolate) {
  TRACE("CJSStackTrace::CJSStackTrace {} stack_trace={}", THIS, stack_trace);
  auto v8_scope = v8u::withScope(m_v8_isolate);
  m_v8_stack_trace.Reset(m_v8_isolate, stack_trace.Handle());
}

v8::Local<v8::StackTrace> CJSStackTrace::Handle() const {
  auto result = v8::Local<v8::StackTrace>::New(m_v8_isolate, m_v8_stack_trace);
  TRACE("CJSStackTrace::Handle {} => {}", THIS, result);
  return result;
}

void CJSStackTrace::Dump(std::ostream& os) const {
  auto v8_scope = v8u::withScope(m_v8_isolate);

  for (int i = 0; i < GetFrameCount(); i++) {
    auto frame = GetFrame(i)->Handle();

    v8::String::Utf8Value funcName(m_v8_isolate, frame->GetFunctionName()),
        scriptName(m_v8_isolate, frame->GetScriptName());

    os << "\tat ";

    if (funcName.length()) {
      os << std::string(*funcName, funcName.length()) << " (";
    }

    if (frame->IsEval()) {
      os << "(eval)";
    } else {
      os << std::string(*scriptName, scriptName.length()) << ":" << frame->GetLineNumber() << ":" << frame->GetColumn();
    }

    if (funcName.length()) {
      os << ")";
    }

    os << std::endl;
  }
}