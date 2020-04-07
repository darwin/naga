#include "JSObjectFunction.h"

#include "PythonObject.h"
#include "JSException.h"
#include "PythonAllowThreadsGuard.h"

py::object CJSObjectFunction::CallWithArgs(py::args py_args, const py::kwargs& py_kwargs) {
  auto args_count = py_args.size();

  if (args_count == 0) {
    throw CJSException("missed self argument", PyExc_TypeError);
  }

  auto py_self = py_args[0];
  if (!py::isinstance<CJSObjectFunction>(py_self)) {
    throw CJSException("self argument must be a js function", PyExc_TypeError);
  }

  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto fn = py::cast<CJSObjectFunctionPtr>(py_self);
  assert(fn.get());
  // TODO: move this into a helper function
  auto raw_args_without_self_tuple = PyTuple_GetSlice(py_args.ptr(), 1, args_count);
  auto py_args_without_self_tuple = py::reinterpret_steal<py::list>(raw_args_without_self_tuple);
  auto py_args_without_self = py::cast<py::list>(py_args_without_self_tuple);
  return fn->Call(fn->Self(), py_args_without_self, py_kwargs);
}

py::object CJSObjectFunction::Call(v8::Local<v8::Object> v8_self, const py::list& py_args, const py::dict& py_kwargs) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);
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

  v8::MaybeLocal<v8::Value> v8_result;

  withPythonAllowThreadsGuard([&]() {
    auto v8_this = v8_self.IsEmpty() ? v8_isolate->GetCurrentContext()->Global() : v8_self;
    v8_result = v8_fn->Call(v8_context, v8_this, v8_params.size(), v8_params.data());
  });

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return CJSObject::Wrap(v8_result.ToLocalChecked());
}

py::object CJSObjectFunction::CreateWithArgs(const CJSObjectFunctionPtr& proto,
                                             const py::tuple& py_args,
                                             const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  if (proto->Object().IsEmpty()) {
    throw CJSException("Object prototype may only be an Object", PyExc_TypeError);
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

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

  return CJSObject::Wrap(v8_result);
}

py::object CJSObjectFunction::ApplyJavascript(const CJSObjectPtr& self,
                                              const py::list& py_args,
                                              const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);
  return Call(self->Object(), py_args, py_kwds);
}

py::object CJSObjectFunction::ApplyPython(py::object py_self, const py::list& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  auto context = v8_isolate->GetCurrentContext();

  return Call(CPythonObject::Wrap(std::move(py_self))->ToObject(context).ToLocalChecked(), py_args, py_kwds);
}

py::object CJSObjectFunction::Invoke(const py::list& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  return Call(Self(), py_args, py_kwds);
}

std::string CJSObjectFunction::GetName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}

void CJSObjectFunction::SetName(const std::string& name) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  func->SetName(
      v8::String::NewFromUtf8(v8_isolate, name.c_str(), v8::NewStringType::kNormal, name.size()).ToLocalChecked());
}

int CJSObjectFunction::GetLineNumber() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptLineNumber();
}

int CJSObjectFunction::GetColumnNumber() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptColumnNumber();
}

std::string CJSObjectFunction::GetResourceName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  return std::string(*name, name.length());
}

std::string CJSObjectFunction::GetInferredName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetInferredName()));

  return std::string(*name, name.length());
}

int CJSObjectFunction::GetLineOffset() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceLineOffset()->Value();
}

int CJSObjectFunction::GetColumnOffset() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceColumnOffset()->Value();
}

py::object CJSObjectFunction::GetOwner2() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);
  return CJSObject::Wrap(Self());
}

CJSObjectFunction::CJSObjectFunction(v8::Local<v8::Object> self, v8::Local<v8::Function> func)
    : CJSObject(func), m_self(v8u::getCurrentIsolate(), self) {}

v8::Local<v8::Object> CJSObjectFunction::Self() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  return v8::Local<v8::Object>::New(v8_isolate, m_self);
}
