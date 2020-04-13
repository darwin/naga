#include "Context.h"
#include "Engine.h"
#include "JSObject.h"
#include "PythonObject.h"
#include "Script.h"
#include "Isolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kContextLogger), __VA_ARGS__)

const int kSelfEmbedderDataIndex = 0;

void CContext::Expose(const py::module& py_module) {
  TRACE("CContext::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CContext, CContextPtr>(py_module, "JSContext", "JSContext is an execution context.")
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

CContextPtr CContext::FromV8(v8::Local<v8::Context> v8_context) {
  // FromV8 may be called only on contexts created by our constructor
  assert(v8_context->GetNumberOfEmbedderDataFields() > kSelfEmbedderDataIndex);
  auto v8_data = v8_context->GetEmbedderData(kSelfEmbedderDataIndex);
  assert(v8_data->IsExternal());
  auto v8_external = v8_data.As<v8::External>();
  auto context_ptr = static_cast<CContext*>(v8_external->Value());
  assert(context_ptr);
  TRACE("CContext::FromV8 v8_context={} => {}", v8_context, (void*)context_ptr);
  return context_ptr->shared_from_this();
}

v8::Local<v8::Context> CContext::ToV8() const {
  auto v8_isolate = m_isolate->ToV8();
  auto v8_result = v8::Local<v8::Context>::New(v8_isolate, m_v8_context);
  TRACE("CContext::ToV8 {} => {}", THIS, v8_result);
  return v8_result;
}

CContext::CContext(const py::object& py_global) : m_py_global(py_global) {
  TRACE("CContext::CContext {} py_global={}", THIS, py_global);
  auto v8_isolate = v8u::getCurrentIsolate();
  m_isolate = CIsolate::FromV8(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_context = v8::Context::New(v8_isolate);
  m_v8_context.Reset(v8_isolate, v8_context);
  auto v8_this = v8::External::New(v8_context->GetIsolate(), this);
  v8_context->SetEmbedderData(kSelfEmbedderDataIndex, v8_this);
  v8::Context::Scope context_scope(v8_context);

  if (!py_global.is_none()) {
    auto v8_proto_key = v8::String::NewFromUtf8(v8_isolate, "__proto__").ToLocalChecked();
    auto v8_global = CPythonObject::Wrap(py_global);
    ToV8()->Global()->Set(v8_context, v8_proto_key, v8_global).Check();
  }
}

CContext::~CContext() {
  TRACE("CContext::~CContext {}", THIS);
  m_v8_context.Reset();
}

py::object CContext::GetGlobal() const {
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::openScope(v8_isolate);
  auto py_result = CJSObject::Wrap(ToV8()->Global());
  TRACE("CContext::GetGlobal {} => {}", THIS, py_result);
  return py_result;
}

py::str CContext::GetSecurityToken() {
  TRACE("CContext::GetSecurityToken {}", THIS);
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_token = ToV8()->GetSecurityToken();

  if (v8_token.IsEmpty()) {
    return py::str();
  }

  auto v8_token_string = v8_token->ToString(m_v8_context.Get(v8_isolate)).ToLocalChecked();
  auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_token_string);
  auto py_result = py::str(*v8_utf, v8_utf.length());
  TRACE("CContext::GetSecurityToken {} => {}", THIS, py_result);
  return py_result;
}

void CContext::SetSecurityToken(const py::str& py_token) const {
  TRACE("CContext::SetSecurityToken {} py_token={}", THIS, py_token);
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (py_token.is_none()) {
    ToV8()->UseDefaultSecurityToken();
  } else {
    std::string str = py::cast<py::str>(py_token);
    auto v8_token_str = v8::String::NewFromUtf8(v8_isolate, str.c_str()).ToLocalChecked();
    ToV8()->SetSecurityToken(v8_token_str);
  }
}

py::object CContext::GetEntered() {
  TRACE("CContext::GetEntered");
  auto v8_isolate = v8u::getCurrentIsolate();
  if (!v8_isolate->InContext()) {
    return py::none();
  }
  auto v8_scope = v8u::openScope(v8_isolate);

  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  if (v8_context.IsEmpty()) {
    return py::none();
  }
  auto py_result = py::cast(FromV8(v8_context));
  TRACE("CContext::GetEntered => {}", py_result);
  return py_result;
}

py::object CContext::GetCurrent() {
  TRACE("CContext::GetCurrent");
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (v8_context.IsEmpty()) {
    return py::none();
  }
  auto py_result = py::cast(FromV8(v8_context));
  TRACE("CContext::GetCurrent => {}", py_result);
  return py_result;
}

py::object CContext::GetCalling() {
  TRACE("CContext::GetCalling");
  auto v8_isolate = v8u::getCurrentIsolate();
  if (!v8_isolate->InContext()) {
    return py::none();
  }
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (v8_context.IsEmpty()) {
    return py::none();
  }

  auto py_result = py::cast(FromV8(v8_context));
  TRACE("CContext::GetCalling => {}", py_result);
  return py_result;
}

py::object CContext::Evaluate(const std::string& src, const std::string& name, int line, int col) {
  TRACE("CContext::Evaluate name={} line={} col={} src={}", name, line, col, src);
  auto v8_isolate = v8u::getCurrentIsolate();
  CEngine engine(v8_isolate);
  CScriptPtr script = engine.Compile(src, name, line, col);
  auto py_result = script->Run();
  TRACE("CContext::Evaluate => {}", py_result);
  return py_result;
}

py::object CContext::EvaluateW(const std::wstring& src, const std::wstring& name, int line, int col) {
  TRACE("CContext::EvaluateW name={} line={} col={} src={}", wstring_printer{name}, line, col, wstring_printer{src});
  auto v8_isolate = v8u::getCurrentIsolate();
  CEngine engine(v8_isolate);
  CScriptPtr script = engine.CompileW(src, name, line, col);
  return script->Run();
}

void CContext::Enter() const {
  TRACE("CContext::Enter {}", THIS);
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::openScope(v8_isolate);
  ToV8()->Enter();
}

void CContext::Leave() const {
  TRACE("CContext::Leave {}", THIS);
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::openScope(v8_isolate);
  ToV8()->Exit();
}

bool CContext::InContext() {
  TRACE("CContext::InContext");
  auto v8_isolate = v8u::getCurrentIsolate();
  return v8_isolate->InContext();
}

void CContext::Dump(std::ostream& os) const {
  fmt::print(os, "CContext {} m_v8_context={} m_py_global={}", THIS, ToV8(), m_py_global);
}

bool CContext::IsEntered() {
  // TODO: this is wrong!
  auto result = !m_v8_context.IsEmpty();
  TRACE("CContext::IsEntered {} => {}", THIS, result);
  return result;
}
