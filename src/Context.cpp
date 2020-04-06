#include "Context.h"
#include "Engine.h"
#include "JSObject.h"
#include "PythonObject.h"

void CContext::Expose(pb::module& m) {
  // clang-format off
  pb::class_<CContext, CContextPtr>(m, "JSContext", "JSContext is an execution context.")
      .def(pb::init<const CContext &>(),
           "Create a new context based on a existing context")
      .def(pb::init<pb::object>())

      .def_property("securityToken", &CContext::GetSecurityToken, &CContext::SetSecurityToken)

      .def_property_readonly("locals", &CContext::GetGlobal,
                             "Local variables within context")

      .def_property_readonly_static(
          "entered", [](pb::object) { return CContext::GetEntered(); },
          "The last entered context.")
      .def_property_readonly_static(
          "current", [](pb::object) { return CContext::GetCurrent(); },
          "The context that is on the top of the stack.")
      .def_property_readonly_static(
          "calling", [](pb::object) { return CContext::GetCalling(); },
          "The context of the calling JavaScript code.")
      .def_property_readonly_static(
          "inContext", [](pb::object) { return CContext::InContext(); },
          "Returns true if V8 has a current context.")

      .def("eval", &CContext::Evaluate,
           pb::arg("source"),
           pb::arg("name") = std::string(),
           pb::arg("line") = -1,
           pb::arg("col") = -1)
      .def("eval", &CContext::EvaluateW,
           pb::arg("source"),
           pb::arg("name") = std::wstring(),
           pb::arg("line") = -1,
           pb::arg("col") = -1)

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

CContext::CContext(const pb::object& py_global) : m_py_global(py_global) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8::Context::New(v8_isolate);
  m_v8_context.Reset(v8_isolate, v8_context);

  v8::Context::Scope context_scope(Handle());

  if (!py_global.is_none()) {
    auto v8_proto_key = v8::String::NewFromUtf8(v8_isolate, "__proto__").ToLocalChecked();
    auto global = CPythonObject::Wrap(py_global);
    auto retcode = Handle()->Global()->Set(v8_context, v8_proto_key, global);
    if (retcode.IsNothing()) {
      // TODO we need to do something if the set call failed
    }

    // TODO: revisit this, we should get rid of all raw refcount operations
    Py_DECREF(py_global.ptr());
  }
}

pb::object CContext::GetGlobal() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  return CJSObject::Wrap(Handle()->Global());
}

pb::str CContext::GetSecurityToken() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_token = Handle()->GetSecurityToken();

  if (v8_token.IsEmpty()) {
    return pb::str();
  }

  auto v8_token_string = v8_token->ToString(m_v8_context.Get(v8_isolate)).ToLocalChecked();
  auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_token_string);
  return pb::str(*v8_utf, v8_utf.length());
}

void CContext::SetSecurityToken(const pb::str& py_token) const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (py_token.is_none()) {
    Handle()->UseDefaultSecurityToken();
  } else {
    std::string str = pb::cast<pb::str>(py_token);
    auto v8_token_str = v8::String::NewFromUtf8(v8_isolate, str.c_str()).ToLocalChecked();
    Handle()->SetSecurityToken(v8_token_str);
  }
}

pb::object CContext::GetEntered() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate->InContext()) {
    return pb::none();
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  if (v8_context.IsEmpty()) {
    return pb::none();
  }
  return pb::cast(CContextPtr(new CContext(v8_context)));
}

pb::object CContext::GetCurrent() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (v8_context.IsEmpty()) {
    return pb::none();
  }
  return pb::cast(CContextPtr(new CContext(v8_context)));
}

pb::object CContext::GetCalling() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate->InContext()) {
    return pb::none();
  }
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (v8_context.IsEmpty()) {
    return pb::none();
  }

  return pb::cast(CContextPtr(new CContext(v8_context)));
}

pb::object CContext::Evaluate(const std::string& src, const std::string& name, int line, int col) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  CEngine engine(v8_isolate);
  CScriptPtr script = engine.Compile(src, name, line, col);
  return script->Run();
}

pb::object CContext::EvaluateW(const std::wstring& src, const std::wstring& name, int line, int col) {
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
