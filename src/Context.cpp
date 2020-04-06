#include "Context.h"
#include "Engine.h"
#include "JSObject.h"
#include "PythonObject.h"
#include "Script.h"

void CContext::Expose(const py::module& py_module) {
  // clang-format off
  py::class_<CContext, CContextPtr>(py_module, "JSContext", "JSContext is an execution context.")
      .def(py::init<const CContext &>(),
           "Create a new context based on a existing context")
      .def(py::init<py::object>())

      .def_property("securityToken", &CContext::GetSecurityToken, &CContext::SetSecurityToken)

      .def_property_readonly("locals", &CContext::GetGlobal,
                             "Local variables within context")

      .def_property_readonly_static("entered", [](const py::object &) { return CContext::GetEntered(); },
                                    "The last entered context.")
      .def_property_readonly_static("current", [](const py::object &) { return CContext::GetCurrent(); },
                                    "The context that is on the top of the stack.")
      .def_property_readonly_static("calling", [](const py::object &) { return CContext::GetCalling(); },
                                    "The context of the calling JavaScript code.")
      .def_property_readonly_static("inContext", [](const py::object &) { return CContext::InContext(); },
                                    "Returns true if V8 has a current context.")

      .def_static("eval", &CContext::Evaluate,
                  py::arg("source"),
                  py::arg("name") = std::string(),
                  py::arg("line") = -1,
                  py::arg("col") = -1)
      .def_static("eval", &CContext::EvaluateW,
                  py::arg("source"),
                  py::arg("name") = std::wstring(),
                  py::arg("line") = -1,
                  py::arg("col") = -1)

      .def("enter", &CContext::Enter,
           "Enter this context. "
           "After entering a context, all code compiled and "
           "run is compiled and run in this context.")
      .def("leave", &CContext::Leave,
           "Exit this context. "
           "Exiting the current context restores the context "
           "that was in place when entering the current context.")

      .def("__bool__", &CContext::IsEntered,
           "the context has been entered.");
  // clang-format on
}

CContext::CContext(const v8::Local<v8::Context>& context) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);

  m_v8_context.Reset(context->GetIsolate(), context);
}

CContext::CContext(const CContext& context) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);

  m_v8_context.Reset(context.Handle()->GetIsolate(), context.Handle());
}

CContext::CContext(const py::object& py_global) : m_py_global(py_global) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8::Context::New(v8_isolate);
  m_v8_context.Reset(v8_isolate, v8_context);

  v8::Context::Scope context_scope(Handle());

  if (!py_global.is_none()) {
    auto v8_proto_key = v8::String::NewFromUtf8(v8_isolate, "__proto__").ToLocalChecked();
    auto global = CPythonObject::Wrap(py_global);
    auto result = Handle()->Global()->Set(v8_context, v8_proto_key, global);
    if (result.IsNothing()) {
      // TODO we need to do something if the set call failed
    }

    // TODO: revisit this, we should get rid of all raw refcount operations
    Py_DECREF(py_global.ptr());
  }
}

py::object CContext::GetGlobal() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  return CJSObject::Wrap(Handle()->Global());
}

py::str CContext::GetSecurityToken() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_token = Handle()->GetSecurityToken();

  if (v8_token.IsEmpty()) {
    return py::str();
  }

  auto v8_token_string = v8_token->ToString(m_v8_context.Get(v8_isolate)).ToLocalChecked();
  auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_token_string);
  return py::str(*v8_utf, v8_utf.length());
}

void CContext::SetSecurityToken(const py::str& py_token) const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (py_token.is_none()) {
    Handle()->UseDefaultSecurityToken();
  } else {
    std::string str = py::cast<py::str>(py_token);
    auto v8_token_str = v8::String::NewFromUtf8(v8_isolate, str.c_str()).ToLocalChecked();
    Handle()->SetSecurityToken(v8_token_str);
  }
}

py::object CContext::GetEntered() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate->InContext()) {
    return py::none();
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  if (v8_context.IsEmpty()) {
    return py::none();
  }
  return py::cast(CContextPtr(new CContext(v8_context)));
}

py::object CContext::GetCurrent() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (v8_context.IsEmpty()) {
    return py::none();
  }
  return py::cast(CContextPtr(new CContext(v8_context)));
}

py::object CContext::GetCalling() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate->InContext()) {
    return py::none();
  }
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (v8_context.IsEmpty()) {
    return py::none();
  }

  return py::cast(CContextPtr(new CContext(v8_context)));
}

py::object CContext::Evaluate(const std::string& src, const std::string& name, int line, int col) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  CEngine engine(v8_isolate);
  CScriptPtr script = engine.Compile(src, name, line, col);
  return script->Run();
}

py::object CContext::EvaluateW(const std::wstring& src, const std::wstring& name, int line, int col) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  CEngine engine(v8_isolate);
  CScriptPtr script = engine.CompileW(src, name, line, col);
  return script->Run();
}

v8::Local<v8::Context> CContext::Handle() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  return v8::Local<v8::Context>::New(v8_isolate, m_v8_context);
}
void CContext::Enter() const {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  Handle()->Enter();
}
void CContext::Leave() const {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  Handle()->Exit();
}

bool CContext::InContext() {
  return v8::Isolate::GetCurrent()->InContext();
}
