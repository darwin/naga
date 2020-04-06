#include "Exception.h"
#include "PythonGIL.h"

std::ostream& operator<<(std::ostream& os, const CJavascriptException& ex) {
  os << "JSError: " << ex.what();

  return os;
}

std::ostream& operator<<(std::ostream& os, const CJavascriptStackTrace& obj) {
  obj.Dump(os);

  return os;
}

void translateJavascriptException(const CJavascriptException& e) {
  CPythonGIL python_gil;

  if (e.GetType()) {
    PyErr_SetString(e.GetType(), e.what());
  } else {
    auto v8_isolate = v8::Isolate::GetCurrent();
    auto v8_scope = v8u::getScope(v8_isolate);

    if (!e.Exception().IsEmpty() && e.Exception()->IsObject()) {
      auto v8_ex = e.Exception()->ToObject(v8_isolate->GetCurrentContext()).ToLocalChecked();

      auto v8_ex_type_key = v8::String::NewFromUtf8(v8_isolate, "exc_type").ToLocalChecked();
      auto v8_ex_type_api = v8::Private::ForApi(v8_isolate, v8_ex_type_key);
      auto v8_ex_type_val = v8_ex->GetPrivate(v8_isolate->GetCurrentContext(), v8_ex_type_api);

      auto v8_ex_value_key = v8::String::NewFromUtf8(v8_isolate, "exc_value").ToLocalChecked();
      auto v8_ex_value_api = v8::Private::ForApi(v8_isolate, v8_ex_value_key);
      auto v8_ex_value_val = v8_ex->GetPrivate(v8_isolate->GetCurrentContext(), v8_ex_value_api);

      if (!v8_ex_type_val.IsEmpty() && !v8_ex_value_val.IsEmpty()) {
        auto v8_ex_type = v8_ex_type_val.ToLocalChecked();
        PyObject* raw_type = nullptr;
        if (v8_ex_type->IsExternal()) {
          auto type_val = v8::Local<v8::External>::Cast(v8_ex_type)->Value();
          raw_type = static_cast<PyObject*>(type_val);
        }
        auto v8_ex_value = v8_ex_value_val.ToLocalChecked();
        PyObject* raw_value = nullptr;
        if (v8_ex_value->IsExternal()) {
          auto value_val = v8::Local<v8::External>::Cast(v8_ex_value)->Value();
          raw_value = static_cast<PyObject*>(value_val);
        }

        // TODO: remove manual refcounting
        if (raw_type && raw_value) {
          PyErr_SetObject(raw_type, raw_value);
          Py_DECREF(raw_type);
          Py_DECREF(raw_value);
          return;
        }
        if (raw_type) {
          Py_DECREF(raw_type);
        }
        if (raw_value) {
          Py_DECREF(raw_value);
        }
      }
    }

    // TODO: revisit if we can do better with pybind
    // Boost Python doesn't support inheriting from Python class,
    // so, just use some workaround to throw our custom exception
    //
    // http://www.language-binding.net/pyplusplus/troubleshooting_guide/exceptions/exceptions.html

    auto m = pb::module::import("_STPyV8");
    auto py_error_class = m.attr("JSError");
    auto py_error_instance = py_error_class(e);
    py_error_instance.inc_ref();
    PyErr_SetObject(py_error_class.ptr(), py_error_instance.ptr());
  }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "performance-unnecessary-value-param"
// TODO: raise question in pybind issues
// clang-tidy suggests "const std::exception_ptr& p" signature but register_exception_translator won't accept it
void translateException(std::exception_ptr p) {
  try {
    if (p) {
      std::rethrow_exception(p);
    }
  } catch (const CJavascriptException& e) {
    translateJavascriptException(e);
  }
}
#pragma clang diagnostic pop

void CJavascriptException::Expose(const pb::module& m) {
  pb::register_exception_translator(&translateException);

  // clang-format off
  pb::class_<CJavascriptStackTrace, CJavascriptStackTracePtr>(m, "JSStackTrace")
      .def("__len__", &CJavascriptStackTrace::GetFrameCount)
      .def("__getitem__", &CJavascriptStackTrace::GetFrame)

          // TODO: .def("__iter__", py::range(&CJavascriptStackTrace::begin, &CJavascriptStackTrace::end))

      .def("__str__", &CJavascriptStackTrace::ToPythonStr);

  pb::enum_<v8::StackTrace::StackTraceOptions>(m, "JSStackTraceOptions")
      .value("LineNumber", v8::StackTrace::kLineNumber)
      .value("ColumnOffset", v8::StackTrace::kColumnOffset)
      .value("ScriptName", v8::StackTrace::kScriptName)
      .value("FunctionName", v8::StackTrace::kFunctionName)
      .value("IsEval", v8::StackTrace::kIsEval)
      .value("IsConstructor", v8::StackTrace::kIsConstructor)
      .value("Overview", v8::StackTrace::kOverview)
      .value("Detailed", v8::StackTrace::kDetailed)
      .export_values();

  pb::class_<CJavascriptStackFrame, CJavascriptStackFramePtr>(m, "JSStackFrame")
      .def_property_readonly("lineNum", &CJavascriptStackFrame::GetLineNumber)
      .def_property_readonly("column", &CJavascriptStackFrame::GetColumn)
      .def_property_readonly("scriptName", &CJavascriptStackFrame::GetScriptName)
      .def_property_readonly("funcName", &CJavascriptStackFrame::GetFunctionName)
      .def_property_readonly("isEval", &CJavascriptStackFrame::IsEval)
      .def_property_readonly("isConstructor", &CJavascriptStackFrame::IsConstructor);

  pb::class_<CJavascriptException>(m, "_JSError")
      .def("__str__", &CJavascriptException::ToPythonStr)

      .def_property_readonly("name", &CJavascriptException::GetName,
                             "The exception name.")
      .def_property_readonly("message", &CJavascriptException::GetMessage,
                             "The exception message.")
      .def_property_readonly("scriptName", &CJavascriptException::GetScriptName,
                             "The script name which throw the exception.")
      .def_property_readonly("lineNum", &CJavascriptException::GetLineNumber, "The line number of error statement.")
      .def_property_readonly("startPos", &CJavascriptException::GetStartPosition,
                             "The start position of error statement in the script.")
      .def_property_readonly("endPos", &CJavascriptException::GetEndPosition,
                             "The end position of error statement in the script.")
      .def_property_readonly("startCol", &CJavascriptException::GetStartColumn,
                             "The start column of error statement in the script.")
      .def_property_readonly("endCol", &CJavascriptException::GetEndColumn,
                             "The end column of error statement in the script.")
      .def_property_readonly("sourceLine", &CJavascriptException::GetSourceLine,
                             "The source line of error statement.")
      .def_property_readonly("stackTrace", &CJavascriptException::GetStackTrace,
                             "The stack trace of error statement.")
      .def("print_tb", &CJavascriptException::PrintCallStack,
           pb::arg("file") = pb::none(),
           "Print the stack trace of error statement.");
  // clang-format on
}

pb::object CJavascriptStackTrace::ToPythonStr() const {
  std::stringstream ss;
  ss << *this;
  return pb::cast(ss.str());
}

CJavascriptStackTracePtr CJavascriptStackTrace::GetCurrentStackTrace(v8::Isolate* v8_isolate,
                                                                     int frame_limit,
                                                                     v8::StackTrace::StackTraceOptions v8_options) {
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_stack_trace = v8::StackTrace::CurrentStackTrace(v8_isolate, frame_limit, v8_options);
  if (v8_stack_trace.IsEmpty()) {
    CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return CJavascriptStackTracePtr(new CJavascriptStackTrace(v8_isolate, v8_stack_trace));
}

int CJavascriptStackTrace::GetFrameCount() const {
  auto v8_scope = v8u::getScope(m_v8_isolate);
  return Handle()->GetFrameCount();
}

CJavascriptStackFramePtr CJavascriptStackTrace::GetFrame(int idx) const {
  auto v8_scope = v8u::getScope(m_v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(m_v8_isolate);
  if (idx >= Handle()->GetFrameCount()) {
    throw CJavascriptException("index of of range", PyExc_IndexError);
  }
  auto v8_stack_frame = Handle()->GetFrame(m_v8_isolate, idx);

  if (v8_stack_frame.IsEmpty()) {
    CJavascriptException::ThrowIf(m_v8_isolate, v8_try_catch);
  }

  return CJavascriptStackFramePtr(new CJavascriptStackFrame(m_v8_isolate, v8_stack_frame));
}

void CJavascriptStackTrace::Dump(std::ostream& os) const {
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

std::string CJavascriptStackFrame::GetScriptName() const {
  v8::HandleScope handle_scope(m_v8_isolate);

  v8::String::Utf8Value name(m_v8_isolate, Handle()->GetScriptName());

  return std::string(*name, name.length());
}

std::string CJavascriptStackFrame::GetFunctionName() const {
  v8::HandleScope handle_scope(m_v8_isolate);

  v8::String::Utf8Value name(m_v8_isolate, Handle()->GetFunctionName());

  return std::string(*name, name.length());
}

std::string CJavascriptException::GetName() {
  if (m_v8_exc.IsEmpty()) {
    return std::string();
  }

  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  v8::String::Utf8Value msg(m_v8_isolate, v8::Local<v8::String>::Cast(
                                              Exception()
                                                  ->ToObject(m_v8_isolate->GetCurrentContext())
                                                  .ToLocalChecked()
                                                  ->Get(m_v8_isolate->GetCurrentContext(),
                                                        v8::String::NewFromUtf8(m_v8_isolate, "name").ToLocalChecked())
                                                  .ToLocalChecked()));

  return std::string(*msg, msg.length());
}

std::string CJavascriptException::GetMessage() {
  if (m_v8_exc.IsEmpty()) {
    return std::string();
  }

  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  v8::String::Utf8Value msg(
      m_v8_isolate,
      v8::Local<v8::String>::Cast(Exception()
                                      ->ToObject(m_v8_isolate->GetCurrentContext())
                                      .ToLocalChecked()
                                      ->Get(m_v8_isolate->GetCurrentContext(),
                                            v8::String::NewFromUtf8(m_v8_isolate, "message").ToLocalChecked())
                                      .ToLocalChecked()));

  return std::string(*msg, msg.length());
}

std::string CJavascriptException::GetScriptName() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  if (!m_v8_msg.IsEmpty() && !Message()->GetScriptResourceName().IsEmpty() &&
      !Message()->GetScriptResourceName()->IsUndefined()) {
    v8::String::Utf8Value name(m_v8_isolate, Message()->GetScriptResourceName());

    return std::string(*name, name.length());
  }

  return std::string();
}

int CJavascriptException::GetLineNumber() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  return m_v8_msg.IsEmpty() ? 1 : Message()->GetLineNumber(m_v8_isolate->GetCurrentContext()).ToChecked();
}

int CJavascriptException::GetStartPosition() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  return m_v8_msg.IsEmpty() ? 1 : Message()->GetStartPosition();
}

int CJavascriptException::GetEndPosition() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  return m_v8_msg.IsEmpty() ? 1 : Message()->GetEndPosition();
}

int CJavascriptException::GetStartColumn() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  return m_v8_msg.IsEmpty() ? 1 : Message()->GetStartColumn();
}

int CJavascriptException::GetEndColumn() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  return m_v8_msg.IsEmpty() ? 1 : Message()->GetEndColumn();
}

std::string CJavascriptException::GetSourceLine() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  if (!m_v8_msg.IsEmpty() && !Message()->GetSourceLine(m_v8_isolate->GetCurrentContext()).IsEmpty()) {
    v8::String::Utf8Value line(m_v8_isolate,
                               Message()->GetSourceLine(m_v8_isolate->GetCurrentContext()).ToLocalChecked());

    return std::string(*line, line.length());
  }

  return std::string();
}

std::string CJavascriptException::GetStackTrace() {
  assert(m_v8_isolate->InContext());

  v8::HandleScope handle_scope(m_v8_isolate);

  if (!m_v8_stack.IsEmpty()) {
    v8::String::Utf8Value stack(m_v8_isolate, v8::Local<v8::String>::Cast(Stack()));

    return std::string(*stack, stack.length());
  }

  return std::string();
}

std::string CJavascriptException::Extract(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch) {
  assert(v8_isolate->InContext());

  v8::HandleScope handle_scope(v8_isolate);

  std::ostringstream oss;

  v8::String::Utf8Value msg(v8_isolate, v8_try_catch.Exception());

  if (*msg) {
    oss << std::string(*msg, msg.length());
  }

  v8::Local<v8::Message> message = v8_try_catch.Message();

  if (!message.IsEmpty()) {
    oss << " ( ";

    if (!message->GetScriptResourceName().IsEmpty() && !message->GetScriptResourceName()->IsUndefined()) {
      v8::String::Utf8Value name(v8_isolate, message->GetScriptResourceName());

      oss << std::string(*name, name.length());
    }

    oss << " @ " << message->GetLineNumber(v8_isolate->GetCurrentContext()).ToChecked() << " : "
        << message->GetStartColumn() << " ) ";

    if (!message->GetSourceLine(v8_isolate->GetCurrentContext()).IsEmpty()) {
      v8::String::Utf8Value line(v8_isolate, message->GetSourceLine(v8_isolate->GetCurrentContext()).ToLocalChecked());

      oss << " -> " << std::string(*line, line.length());
    }
  }

  return oss.str();
}

static struct {
  const char* name;
  PyObject* type;
} SupportErrors[] = {{"RangeError", PyExc_IndexError},
                     {"ReferenceError", PyExc_ReferenceError},
                     {"SyntaxError", PyExc_SyntaxError},
                     {"TypeError", PyExc_TypeError}};

void CJavascriptException::ThrowIf(v8::Isolate* v8_isolate, const v8::TryCatch& v8_try_catch) {
  auto v8_scope = v8u::getScope(v8_isolate);
  if (!v8_try_catch.HasCaught() || !v8_try_catch.CanContinue()) {
    return;
  }

  PyObject* raw_type = nullptr;
  auto v8_ex = v8_try_catch.Exception();

  if (v8_ex->IsObject()) {
    auto v8_context = v8_isolate->GetCurrentContext();
    auto v8_exc_obj = v8_ex->ToObject(v8_context).ToLocalChecked();
    auto v8_name = v8::String::NewFromUtf8(v8_isolate, "name").ToLocalChecked();

    if (v8_exc_obj->Has(v8_context, v8_name).ToChecked()) {
      auto v8_name_val = v8_exc_obj->Get(v8_isolate->GetCurrentContext(), v8_name).ToLocalChecked();
      auto v8_uft = v8u::toUtf8Value(v8_isolate, v8_name_val.As<v8::String>());

      for (auto& SupportError : SupportErrors) {
        if (strncasecmp(SupportError.name, *v8_uft, v8_uft.length()) == 0) {
          raw_type = SupportError.type;
        }
      }
    }
  }

  throw CJavascriptException(v8_isolate, v8_try_catch, raw_type);
}

void CJavascriptException::PrintCallStack(pb::object py_file) {
  CPythonGIL python_gil;

  // TODO: move this into utility function
  auto raw_file = py_file.is_none() ? PySys_GetObject("stdout") : py_file.ptr();
  auto fd = PyObject_AsFileDescriptor(raw_file);

  Message()->PrintCurrentStackTrace(m_v8_isolate, fdopen(fd, "w+"));
}

pb::object CJavascriptException::ToPythonStr() const {
  std::stringstream ss;
  ss << *this;
  return pb::cast(ss.str());
}
