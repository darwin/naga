#include "JSObjectFunction.h"
#include "PythonObject.h"
#include "PythonDateTime.h"
#include "JSObjectCLJS.h"
#include "Exception.h"
#include "PythonAllowThreadsGuard.h"

py::object CJSObjectFunction::CallWithArgs(py::tuple args, py::dict kwds) {
  size_t argc = ::PyTuple_Size(args.ptr());

  if (argc == 0)
    throw CJavascriptException("missed self argument", ::PyExc_TypeError);

  py::object self = args[0];
  py::extract<CJSObjectFunction&> extractor(self);

  if (!extractor.check())
    throw CJavascriptException("missed self argument", ::PyExc_TypeError);

  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::TryCatch try_catch(isolate);

  CJSObjectFunction& func = extractor();
  py::list argv(args.slice(1, py::_));

  return func.Call(func.Self(), argv, kwds);
}

py::object CJSObjectFunction::Call(v8::Local<v8::Object> self, py::list args, py::dict kwds) {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  size_t args_count = ::PyList_Size(args.ptr()), kwds_count = ::PyMapping_Size(kwds.ptr());

  std::vector<v8::Local<v8::Value> > params(args_count + kwds_count);

  for (size_t i = 0; i < args_count; i++) {
    params[i] = CPythonObject::Wrap(args[i]);
  }

  py::list values = kwds.values();

  for (size_t i = 0; i < kwds_count; i++) {
    params[args_count + i] = CPythonObject::Wrap(values[i]);
  }

  v8::MaybeLocal<v8::Value> result;

  withPythonAllowThreadsGuard([&]() {
    result = func->Call(context, self.IsEmpty() ? isolate->GetCurrentContext()->Global() : self, params.size(),
                        params.empty() ? NULL : &params[0]);
  });

  if (result.IsEmpty())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return CJSObject::Wrap(result.ToLocalChecked());
}

py::object CJSObjectFunction::CreateWithArgs(CJavascriptFunctionPtr proto, py::tuple args, py::dict kwds) {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  if (proto->Object().IsEmpty())
    throw CJavascriptException("Object prototype may only be an Object", ::PyExc_TypeError);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(proto->Object());

  size_t args_count = ::PyTuple_Size(args.ptr());

  std::vector<v8::Local<v8::Value> > params(args_count);

  for (size_t i = 0; i < args_count; i++) {
    params[i] = CPythonObject::Wrap(args[i]);
  }

  v8::Local<v8::Object> result;

  withPythonAllowThreadsGuard([&]() {
    result = func->NewInstance(context, params.size(), params.empty() ? NULL : &params[0]).ToLocalChecked();
  });

  if (result.IsEmpty())
    CJavascriptException::ThrowIf(isolate, try_catch);

  size_t kwds_count = ::PyMapping_Size(kwds.ptr());
  py::list items = kwds.items();

  for (size_t i = 0; i < kwds_count; i++) {
    py::tuple item(items[i]);

    py::str key(item[0]);
    py::object value = item[1];

    result->Set(context, v8u::toString(key), CPythonObject::Wrap(value)).Check();
  }

  return CJSObject::Wrap(result);
}

py::object CJSObjectFunction::ApplyJavascript(CJSObjectPtr self, py::list args, py::dict kwds) {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  return Call(self->Object(), args, kwds);
}

py::object CJSObjectFunction::ApplyPython(py::object self, py::list args, py::dict kwds) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8u::checkContext(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  return Call(CPythonObject::Wrap(self)->ToObject(context).ToLocalChecked(), args, kwds);
}

py::object CJSObjectFunction::Invoke(py::list args, py::dict kwds) {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  return Call(Self(), args, kwds);
}

const std::string CJSObjectFunction::GetName(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}

void CJSObjectFunction::SetName(const std::string& name) {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  func->SetName(
      v8::String::NewFromUtf8(isolate, name.c_str(), v8::NewStringType::kNormal, name.size()).ToLocalChecked());
}

int CJSObjectFunction::GetLineNumber(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptLineNumber();
}

int CJSObjectFunction::GetColumnNumber(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptColumnNumber();
}

const std::string CJSObjectFunction::GetResourceName(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  return std::string(*name, name.length());
}

const std::string CJSObjectFunction::GetInferredName(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetInferredName()));

  return std::string(*name, name.length());
}

int CJSObjectFunction::GetLineOffset(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceLineOffset()->Value();
}

int CJSObjectFunction::GetColumnOffset(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceColumnOffset()->Value();
}

py::object CJSObjectFunction::GetOwner(void) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  return CJSObject::Wrap(Self());
}