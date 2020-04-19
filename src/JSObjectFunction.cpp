#include "JSObject.h"

#include "PythonObject.h"
#include "JSException.h"
#include "PythonAllowThreadsGuard.h"

py::object CJSObject::PythonCallWithArgs(py::args py_args, const py::kwargs& py_kwargs) {
  auto args_count = py_args.size();

  if (args_count == 0) {
    throw CJSException("missed self argument", PyExc_TypeError);
  }

  auto py_self = py_args[0];

  if (!py::isinstance<CJSObject>(py_self)) {
    throw CJSException("self argument must be a JSObject", PyExc_TypeError);
  }

  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto fn = py::cast<CJSObjectPtr>(py_self);
  assert(fn.get());
  if (!fn->HasRole(CJSObject::Roles::JSFunction)) {
    throw CJSException("self argument must be a js function", PyExc_TypeError);
  }

  // TODO: move this into a helper function
  auto raw_args_without_self_tuple = PyTuple_GetSlice(py_args.ptr(), 1, args_count);
  auto py_args_without_self_tuple = py::reinterpret_steal<py::list>(raw_args_without_self_tuple);
  auto py_args_without_self = py::cast<py::list>(py_args_without_self_tuple);
  return fn->Call(py_args_without_self, py_kwargs);
}

py::object CJSObject::Call(const py::list& py_args,
                           const py::dict& py_kwargs,
                           std::optional<v8::Local<v8::Object>> opt_v8_this) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);
  auto v8_fn = Object().As<v8::Function>();

  auto args_count = py_args.size();
  auto kwargs_count = py_kwargs.size();

  std::vector<v8::Local<v8::Value>> v8_params(args_count + kwargs_count);

  size_t i;
  for (i = 0; i < args_count; i++) {
    v8_params[i] = CPythonObject::Wrap(py_args[i]);
  }

  i = 0;
  auto it = py_kwargs.begin();
  while (it != py_kwargs.end()) {
    v8_params[args_count + i] = CPythonObject::Wrap(it->second());
    i++;
    it++;
  }

  auto v8_result = withPythonAllowThreadsGuard([&]() {
    if (!opt_v8_this) {
      return v8_fn->Call(v8_context, v8_context->Global(), v8_params.size(), v8_params.data());
    } else {
      auto v8_unbound_val = v8_fn->GetBoundFunction();
      if (v8_unbound_val->IsUndefined()) {
        return v8_fn->Call(v8_context, *opt_v8_this, v8_params.size(), v8_params.data());
      } else {
        assert(v8_unbound_val->IsFunction());
        auto v8_unbound_fn = v8_unbound_val.As<v8::Function>();
        return v8_unbound_fn->Call(v8_context, *opt_v8_this, v8_params.size(), v8_params.data());
      }
    }
  });

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return CJSObject::Wrap(v8_isolate, v8_result.ToLocalChecked());
}

py::object CJSObject::PythonCreateWithArgs(const CJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  if (proto->Object().IsEmpty()) {
    throw CJSException("Object prototype may only be an Object", PyExc_TypeError);
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto fn = proto->Object().As<v8::Function>();
  auto args_count = py_args.size();
  std::vector<v8::Local<v8::Value>> v8_params(args_count);

  for (size_t i = 0; i < args_count; i++) {
    v8_params[i] = CPythonObject::Wrap(py_args[i]);
  }

  v8::Local<v8::Object> v8_result;

  withPythonAllowThreadsGuard(
      [&]() { v8_result = fn->NewInstance(v8_context, v8_params.size(), v8_params.data()).ToLocalChecked(); });

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  auto it = py_kwds.begin();
  while (it != py_kwds.end()) {
    auto py_key = it->first;
    auto py_val = it->second;
    auto v8_key = v8u::toString(py_key);
    auto v8_val = CPythonObject::Wrap(py_val);
    v8_result->Set(v8_context, v8_key, v8_val).Check();
    it++;
  }

  return CJSObject::Wrap(v8_isolate, v8_result);
}

py::object CJSObject::ApplyJavascript(const CJSObjectPtr& self, const py::list& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  return Call(py_args, py_kwds, self->Object());
}

py::object CJSObject::ApplyPython(py::object py_self, const py::list& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_this = CPythonObject::Wrap(std::move(py_self))->ToObject(v8_context).ToLocalChecked();
  return Call(py_args, py_kwds, v8_this);
}

py::object CJSObject::Invoke(const py::list& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  return Call(py_args, py_kwds);
}

std::string CJSObject::GetName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}

void CJSObject::SetName(const std::string& name) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  func->SetName(
      v8::String::NewFromUtf8(v8_isolate, name.c_str(), v8::NewStringType::kNormal, name.size()).ToLocalChecked());
}

int CJSObject::GetLineNumber() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptLineNumber();
}

int CJSObject::GetColumnNumber() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptColumnNumber();
}

std::string CJSObject::GetResourceName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  return std::string(*name, name.length());
}

std::string CJSObject::GetInferredName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetInferredName()));

  return std::string(*name, name.length());
}

int CJSObject::GetLineOffset() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceLineOffset()->Value();
}

int CJSObject::GetColumnOffset() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceColumnOffset()->Value();
}