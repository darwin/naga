#include "JSStackTrace.h"
#include "JSStackFrame.h"
#include "JSException.h"

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& obj) {
  obj.Dump(os);

  return os;
}

void CJSStackTrace::Expose(const py::module& m) {
  // clang-format off
  py::class_<CJSStackTrace, CJSStackTracePtr>(m, "JSStackTrace")
      .def("__len__", &CJSStackTrace::GetFrameCount)
      .def("__getitem__", &CJSStackTrace::GetFrame)

          // TODO: .def("__iter__", py::range(&CJavascriptStackTrace::begin, &CJavascriptStackTrace::end))

      .def("__str__", &CJSStackTrace::ToPythonStr);

  py::enum_<v8::StackTrace::StackTraceOptions>(m, "JSStackTraceOptions")
      .value("LineNumber", v8::StackTrace::kLineNumber)
      .value("ColumnOffset", v8::StackTrace::kColumnOffset)
      .value("ScriptName", v8::StackTrace::kScriptName)
      .value("FunctionName", v8::StackTrace::kFunctionName)
      .value("IsEval", v8::StackTrace::kIsEval)
      .value("IsConstructor", v8::StackTrace::kIsConstructor)
      .value("Overview", v8::StackTrace::kOverview)
      .value("Detailed", v8::StackTrace::kDetailed)
      .export_values();
  // clang-format on
}

py::object CJSStackTrace::ToPythonStr() const {
  std::stringstream ss;
  ss << *this;
  return py::cast(ss.str());
}

CJSStackTracePtr CJSStackTrace::GetCurrentStackTrace(v8::Isolate* v8_isolate,
                                                     int frame_limit,
                                                     v8::StackTrace::StackTraceOptions v8_options) {
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_stack_trace = v8::StackTrace::CurrentStackTrace(v8_isolate, frame_limit, v8_options);
  if (v8_stack_trace.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return CJSStackTracePtr(new CJSStackTrace(v8_isolate, v8_stack_trace));
}

int CJSStackTrace::GetFrameCount() const {
  auto v8_scope = v8u::getScope(m_v8_isolate);
  return Handle()->GetFrameCount();
}

CJSStackFramePtr CJSStackTrace::GetFrame(int idx) const {
  auto v8_scope = v8u::getScope(m_v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(m_v8_isolate);
  if (idx >= Handle()->GetFrameCount()) {
    throw CJSException("index of of range", PyExc_IndexError);
  }
  auto v8_stack_frame = Handle()->GetFrame(m_v8_isolate, idx);

  if (v8_stack_frame.IsEmpty()) {
    CJSException::ThrowIf(m_v8_isolate, v8_try_catch);
  }

  return CJSStackFramePtr(new CJSStackFrame(m_v8_isolate, v8_stack_frame));
}

void CJSStackTrace::Dump(std::ostream& os) const {
  v8::HandleScope handle_scope(m_v8_isolate);

  v8::TryCatch try_catch(m_v8_isolate);

  std::ostringstream oss;

  for (int i = 0; i < GetFrameCount(); i++) {
    v8::Local<v8::StackFrame> frame = GetFrame(i)->Handle();

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
CJSStackTrace::CJSStackTrace(v8::Isolate* v8_isolate, v8::Local<v8::StackTrace> v8_stack_trace)
    : m_v8_isolate(v8_isolate), m_v8_stack_trace(v8_isolate, v8_stack_trace) {}

CJSStackTrace::CJSStackTrace(const CJSStackTrace& stack_trace) : m_v8_isolate(stack_trace.m_v8_isolate) {
  auto v8_scope = v8u::getScope(m_v8_isolate);
  m_v8_stack_trace.Reset(m_v8_isolate, stack_trace.Handle());
}

v8::Local<v8::StackTrace> CJSStackTrace::Handle() const {
  return v8::Local<v8::StackTrace>::New(m_v8_isolate, m_v8_stack_trace);
}
