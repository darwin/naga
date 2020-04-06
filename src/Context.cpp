#include "Context.h"
#include "Platform.h"
#include "Isolate.h"
#include "Engine.h"
#include "JSObject.h"
#include "PythonObject.h"

void CContext::Expose(pb::module& m) {
  pb::class_<CPlatform, CPlatformPtr>(m, "JSPlatform", "JSPlatform allows the V8 platform to be initialized")
      .def(pb::init<std::string>(), pb::arg("argv") = std::string())
      .def("init", &CPlatform::Init, "Initializes the platform");

  pb::class_<CIsolate, CIsolatePtr>(m, "JSIsolate", "JSIsolate is an isolated instance of the V8 engine.")
      .def(pb::init<bool>(), pb::arg("owner") = false)

      .def_property_readonly_static(
          "current", [](pb::object) { return CIsolate::GetCurrent(); },
          "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")

      .def_property_readonly("locked", &CIsolate::IsLocked)

      .def("GetCurrentStackTrace", &CIsolate::GetCurrentStackTrace)

      .def("enter", &CIsolate::Enter,
           "Sets this isolate as the entered one for the current thread. "
           "Saves the previously entered one (if any), so that it can be "
           "restored when exiting.  Re-entering an isolate is allowed.")

      .def("leave", &CIsolate::Leave,
           "Exits this isolate by restoring the previously entered one in the current thread. "
           "The isolate may still stay the same, if it was entered more than once.");

  //    py::class_<CContext, boost::noncopyable>("JSContext", "JSContext is an execution context.", py::no_init)
  //        .def(py::init<const CContext&>("Create a new context based on a existing context"))
  //        .def(py::init<py::object>((py::arg("global") = py::object()), "Create a new context based on global
  //        object"))
  //
  //        .add_property("securityToken", &CContext::GetSecurityToken, &CContext::SetSecurityToken)
  //
  //        .add_property("locals", &CContext::GetGlobal, "Local variables within context")
  //
  //        .add_static_property("entered", &CContext::GetEntered, "The last entered context.")
  //        .add_static_property("current", &CContext::GetCurrent, "The context that is on the top of the stack.")
  //        .add_static_property("calling", &CContext::GetCalling, "The context of the calling JavaScript code.")
  //        .add_static_property("inContext", &CContext::InContext, "Returns true if V8 has a current context.")
  //
  //        .def("eval", &CContext::Evaluate,
  //             (py::arg("source"), py::arg("name") = std::string(), py::arg("line") = -1, py::arg("col") = -1))
  //        .def("eval", &CContext::EvaluateW,
  //             (py::arg("source"), py::arg("name") = std::wstring(), py::arg("line") = -1, py::arg("col") = -1))
  //
  //        .def("enter", &CContext::Enter,
  //             "Enter this context. "
  //             "After entering a context, all code compiled and "
  //             "run is compiled and run in this context.")
  //        .def("leave", &CContext::Leave,
  //             "Exit this context. "
  //             "Exiting the current context restores the context "
  //             "that was in place when entering the current context.")
  //
  //        .def("__bool__", &CContext::IsEntered, "the context has been entered.");
  //  py::objects::class_value_wrapper<
  //      std::shared_ptr<CContext>,
  //      py::objects::make_ptr_instance<CContext, py::objects::pointer_holder<std::shared_ptr<CContext>, CContext> >
  //  >();

  pb::class_<CContext, CContextPtr>(m, "JSContext", "JSContext is an execution context.")
      .def(pb::init<const CContext&>(), "Create a new context based on a existing context")
      .def(pb::init<pb::object>())

      .def_property("securityToken", &CContext::GetSecurityToken2, &CContext::SetSecurityToken2)

      .def_property_readonly("locals", &CContext::GetGlobal, "Local variables within context")

      .def_property_readonly_static(
          "entered", [](pb::object) { return CContext::GetEntered2(); }, "The last entered context.")
      .def_property_readonly_static(
          "current", [](pb::object) { return CContext::GetCurrent2(); }, "The context that is on the top of the stack.")
      .def_property_readonly_static(
          "calling", [](pb::object) { return CContext::GetCalling2(); }, "The context of the calling JavaScript code.")
      .def_property_readonly_static(
          "inContext", [](pb::object) { return CContext::InContext(); }, "Returns true if V8 has a current context.")

      .def("eval", &CContext::Evaluate2, pb::arg("source"), pb::arg("name") = std::string(), pb::arg("line") = -1,
           pb::arg("col") = -1)
      .def("eval", &CContext::EvaluateW2, pb::arg("source"), pb::arg("name") = std::wstring(), pb::arg("line") = -1,
           pb::arg("col") = -1)

      .def("enter", &CContext::Enter,
           "Enter this context. "
           "After entering a context, all code compiled and "
           "run is compiled and run in this context.")
      .def("leave", &CContext::Leave,
           "Exit this context. "
           "Exiting the current context restores the context "
           "that was in place when entering the current context.")

      .def("__bool__", &CContext::IsEntered, "the context has been entered.");

  //  py::objects::class_value_wrapper<
  //      std::shared_ptr<CPlatform>,
  //      py::objects::make_ptr_instance<CPlatform,
  //                                     py::objects::pointer_holder<std::shared_ptr<CPlatform>, CPlatform> > >();
  //
  //  py::objects::class_value_wrapper<
  //      std::shared_ptr<CIsolate>,
  //      py::objects::make_ptr_instance<CIsolate, py::objects::pointer_holder<std::shared_ptr<CIsolate>, CIsolate> >
  //      >();
  //
}

CContext::CContext(v8::Local<v8::Context> context) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  m_context.Reset(context->GetIsolate(), context);
}

CContext::CContext(const CContext& context) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  m_context.Reset(context.Handle()->GetIsolate(), context.Handle());
}

// CContext::CContext(const py::object& global) : m_global(global) {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = v8::Context::New(isolate);
//
//  m_context.Reset(isolate, context);
//
//  v8::Context::Scope context_scope(Handle());
//
//  if (!global.is_none()) {
//    v8::Maybe<bool> retcode = Handle()->Global()->Set(
//        context, v8::String::NewFromUtf8(isolate, "__proto__").ToLocalChecked(), CPythonObject::Wrap(global));
//    if (retcode.IsNothing()) {
//      // TODO we need to do something if the set call failed
//    }
//
//    Py_DECREF(global.ptr());
//  }
//}

CContext::CContext(const pb::object& global) : m_global(global) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8::Context::New(v8_isolate);
  m_context.Reset(v8_isolate, v8_context);

  v8::Context::Scope context_scope(Handle());

  if (!global.is_none()) {
    auto v8_proto_key = v8::String::NewFromUtf8(v8_isolate, "__proto__").ToLocalChecked();
    auto wrapped_obj = CPythonObject::Wrap(global);
//    std::cerr << "CONTEXT with GLOBAL: " << wrapped_obj << "\n";
    assert(CPythonObject::IsWrapped2(wrapped_obj.As<v8::Object>()));
    v8::Maybe<bool> retcode = Handle()->Global()->Set(v8_context, v8_proto_key, wrapped_obj);
//    if (retcode.IsNothing()) {
//      // TODO we need to do something if the set call failed
//    }

    Py_DECREF(global.ptr());
  }
}

// py::object CContext::GetGlobal() const {
//  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
//
//  return CJSObject::Wrap(Handle()->Global());
//}

pb::object CContext::GetGlobal() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  return CJSObject::Wrap(Handle()->Global());
}

//py::str CContext::GetSecurityToken() {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Value> token = Handle()->GetSecurityToken();
//
//  if (token.IsEmpty())
//    return py::str();
//
//  v8::String::Utf8Value str(isolate, token->ToString(m_context.Get(isolate)).ToLocalChecked());
//
//  return py::str(*str, str.length());
//}

//void CContext::SetSecurityToken(const py::str& token) const {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  if (token.is_none()) {
//    Handle()->UseDefaultSecurityToken();
//  } else {
//    Handle()->SetSecurityToken(v8::String::NewFromUtf8(isolate, py::extract<const char*>(token)()).ToLocalChecked());
//  }
//}

pb::str CContext::GetSecurityToken2() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);

  v8::Local<v8::Value> token = Handle()->GetSecurityToken();

  if (token.IsEmpty()) {
    return pb::str();
  }

  auto v8_token_string = token->ToString(m_context.Get(v8_isolate)).ToLocalChecked();
  auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_token_string);
  return pb::str(*v8_utf, v8_utf.length());
}

void CContext::SetSecurityToken2(const pb::str& py_token) const {
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

//py::object CContext::GetEntered() {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> entered = isolate->GetEnteredOrMicrotaskContext();
//
//  return (!isolate->InContext() || entered.IsEmpty())
//             ? py::object()
//             : py::object(py::handle<>(
//                   boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(entered)))));
//}

pb::object CContext::GetEntered2() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate->InContext()) {
    return pb::none();
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  if (v8_context.IsEmpty()) {
    return pb::none();
  }
  auto context = new CContext(v8_context);
  return pb::cast(context);
}

//py::object CContext::GetCurrent() {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//  v8::Local<v8::Context> current = isolate->GetCurrentContext();
//
//  return (current.IsEmpty()) ? py::object()
//                             : py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CContext>(
//                                   CContextPtr(new CContext(current)))));
//}

pb::object CContext::GetCurrent2() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);

  auto v8_context = v8_isolate->GetCurrentContext();
  if (v8_context.IsEmpty()) {
    return pb::none();
  }
  auto context = new CContext(v8_context);
  return pb::cast(context);
}

//py::object CContext::GetCalling() {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> calling = isolate->GetCurrentContext();
//
//  return (!isolate->InContext() || calling.IsEmpty())
//             ? py::object()
//             : py::object(py::handle<>(
//                   boost::python::converter::shared_ptr_to_python<CContext>(CContextPtr(new CContext(calling)))));
//}

pb::object CContext::GetCalling2() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate->InContext()) {
    return pb::none();
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  auto v8_context = v8_isolate->GetCurrentContext();
  if (v8_context.IsEmpty()) {
    return pb::none();
  }

  auto context = new CContext(v8_context);
  return pb::cast(context);
}

//py::object CContext::Evaluate(const std::string& src, const std::string& name, int line, int col) {
//  CEngine engine(v8::Isolate::GetCurrent());
//
//  CScriptPtr script = engine.Compile(src, name, line, col);
//
//  return script->Run();
//}

pb::object CContext::Evaluate2(const std::string& src, const std::string& name, int line, int col) {
  CEngine engine(v8::Isolate::GetCurrent());
  CScriptPtr script = engine.Compile(src, name, line, col);
  return script->Run2();
}

//py::object CContext::EvaluateW(const std::wstring& src, const std::wstring& name, int line, int col) {
//  CEngine engine(v8::Isolate::GetCurrent());
//
//  CScriptPtr script = engine.CompileW(src, name, line, col);
//
//  return script->Run();
//}

pb::object CContext::EvaluateW2(const std::wstring& src, const std::wstring& name, int line, int col) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  CEngine engine(v8_isolate);
  CScriptPtr script = engine.CompileW(src, name, line, col);
  return script->Run2();
}