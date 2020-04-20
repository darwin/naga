#include "JSObjectFunctionImpl.h"
#include "PythonObject.h"
#include "JSException.h"
#include "PythonAllowThreadsGuard.h"

py::object CJSObjectFunctionImpl::Call(const py::list& py_args,
                                       const py::dict& py_kwargs,
                                       std::optional<v8::Local<v8::Object>> opt_v8_this) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);
  auto v8_fn = m_base.Object().As<v8::Function>();

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

py::object CJSObjectFunctionImpl::Apply(const py::object& py_self, const py::list& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_this = CPythonObject::Wrap(std::move(py_self))->ToObject(v8_context).ToLocalChecked();
  return Call(py_args, py_kwds, v8_this);
}

std::string CJSObjectFunctionImpl::GetName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}

void CJSObjectFunctionImpl::SetName(const std::string& name) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  func->SetName(
      v8::String::NewFromUtf8(v8_isolate, name.c_str(), v8::NewStringType::kNormal, name.size()).ToLocalChecked());
}

int CJSObjectFunctionImpl::GetLineNumber() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  return func->GetScriptLineNumber();
}

int CJSObjectFunctionImpl::GetColumnNumber() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  return func->GetScriptColumnNumber();
}

std::string CJSObjectFunctionImpl::GetResourceName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  return std::string(*name, name.length());
}

std::string CJSObjectFunctionImpl::GetInferredName() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetInferredName()));

  return std::string(*name, name.length());
}

int CJSObjectFunctionImpl::GetLineOffset() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  return func->GetScriptOrigin().ResourceLineOffset()->Value();
}

int CJSObjectFunctionImpl::GetColumnOffset() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.Object());

  return func->GetScriptOrigin().ResourceColumnOffset()->Value();
}