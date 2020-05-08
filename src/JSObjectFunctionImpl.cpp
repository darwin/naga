#include "JSObjectFunctionImpl.h"
#include "JSException.h"
#include "PythonThreads.h"
#include "Wrapping.h"
#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectFunctionImplLogger), __VA_ARGS__)

py::object JSObjectFunctionImpl::Call(const py::list& py_args,
                                      const py::dict& py_kwargs,
                                      std::optional<v8::Local<v8::Object>> opt_v8_this) const {
  TRACE("JSObjectFunctionImpl::Call {} py_args={} py_kwargs={}", THIS, py_args, py_kwargs);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_fn = m_base.ToV8(v8_isolate).As<v8::Function>();

  auto args_count = py_args.size();
  auto kwargs_count = py_kwargs.size();

  std::vector<v8::Local<v8::Value>> v8_params(args_count + kwargs_count);

  size_t i;
  for (i = 0; i < args_count; i++) {
    v8_params[i] = wrap(py_args[i]);
  }

  i = 0;
  auto it = py_kwargs.begin();
  while (it != py_kwargs.end()) {
    v8_params[args_count + i] = wrap(it->second());
    i++;
    it++;
  }

  auto v8_result = withAllowedPythonThreads([&] {
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

  return wrap(v8_isolate, v8_result.ToLocalChecked());
}

py::object JSObjectFunctionImpl::Apply(const py::object& py_self,
                                       const py::list& py_args,
                                       const py::dict& py_kwds) const {
  TRACE("JSObjectFunctionImpl::Apply {} py_self={} py_args={} py_kwds={}", THIS, py_self, py_args, py_kwds);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_this = wrap(std::move(py_self))->ToObject(v8_context).ToLocalChecked();
  return Call(py_args, py_kwds, v8_this);
}

std::string JSObjectFunctionImpl::GetName() const {
  TRACE("JSObjectFunctionImpl::GetName {}", THIS);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}

void JSObjectFunctionImpl::SetName(const std::string& name) const {
  TRACE("JSObjectFunctionImpl::SetName {} => {}", THIS, name);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  func->SetName(
      v8::String::NewFromUtf8(v8_isolate, name.c_str(), v8::NewStringType::kNormal, name.size()).ToLocalChecked());
}

int JSObjectFunctionImpl::GetLineNumber() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  auto result = func->GetScriptLineNumber();
  TRACE("JSObjectFunctionImpl::GetLineNumber {} => {}", THIS, result);
  return result;
}

int JSObjectFunctionImpl::GetColumnNumber() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  auto result = func->GetScriptColumnNumber();
  TRACE("JSObjectFunctionImpl::GetColumnNumber {} => {}", THIS, result);
  return result;
}

int JSObjectFunctionImpl::GetLineOffset() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  auto result = func->GetScriptOrigin().ResourceLineOffset()->Value();
  TRACE("JSObjectFunctionImpl::GetLineOffset {} => {}", THIS, result);
  return result;
}

int JSObjectFunctionImpl::GetColumnOffset() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  auto result = func->GetScriptOrigin().ResourceColumnOffset()->Value();
  TRACE("JSObjectFunctionImpl::GetColumnOffset {} => {}", THIS, result);
  return result;
}

std::string JSObjectFunctionImpl::GetResourceName() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  auto result = std::string(*name, name.length());
  TRACE("JSObjectFunctionImpl::GetResourceName {} => {}", THIS, result);
  return result;
}

std::string JSObjectFunctionImpl::GetInferredName() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(m_base.ToV8(v8_isolate));

  v8::String::Utf8Value name(v8_isolate, v8::Local<v8::String>::Cast(func->GetInferredName()));

  auto result = std::string(*name, name.length());
  TRACE("JSObjectFunctionImpl::GetInferredName {} => {}", THIS, result);
  return result;
}
