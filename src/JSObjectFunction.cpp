#include "JSObjectFunction.h"
#include "PythonObject.h"
#include "PythonDateTime.h"
#include "JSObjectCLJS.h"
#include "JSException.h"
#include "PythonAllowThreadsGuard.h"

//py::object CJSObjectFunction::CallWithArgs(py::tuple args, py::dict kwds) {
//  size_t argc = PyTuple_Size(args.ptr());
//
//  if (argc == 0)
//    throw CJavascriptException("missed self argument", PyExc_TypeError);
//
//  py::object self = args[0];
//  py::extract<CJSObjectFunction&> extractor(self);
//
//  if (!extractor.check())
//    throw CJavascriptException("missed self argument", PyExc_TypeError);
//
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::TryCatch try_catch(isolate);
//
//  CJSObjectFunction& func = extractor();
//  py::list argv(args.slice(1, py::_));
//
//  return func.Call(func.Self(), argv, kwds);
//}

pb::object CJSObjectFunction::CallWithArgs2(pb::args py_args, pb::kwargs py_kwargs) {
  auto args_count = py_args.size();

  if (args_count == 0) {
    throw CJSException("missed self argument", PyExc_TypeError);
  }

  auto self = py_args[0];
  if (!pb::isinstance<CJSObjectFunction>(self)) {
    throw CJSException("self argument must be a js function", PyExc_TypeError);
  }

  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto fn = pb::cast<CJSObjectFunctionPtr>(self);
  assert(fn.get());
  // TODO: move this into a helper function
  auto raw_args_without_self_tuple = PyTuple_GetSlice(py_args.ptr(), 1, args_count);
  auto py_args_without_self_tuple = pb::reinterpret_steal<pb::list>(raw_args_without_self_tuple);
  auto py_args_without_self = pb::cast<pb::list>(py_args_without_self_tuple);
  return fn->Call2(fn->Self(), py_args_without_self, py_kwargs);
}

//py::object CJSObjectFunction::Call(v8::Local<v8::Object> self, py::list args, py::dict kwds) {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());
//
//  size_t args_count = PyList_Size(args.ptr()), kwds_count = PyMapping_Size(kwds.ptr());
//
//  std::vector<v8::Local<v8::Value>> params(args_count + kwds_count);
//
//  for (size_t i = 0; i < args_count; i++) {
//    params[i] = CPythonObject::Wrap(args[i]);
//  }
//
//  py::list values = kwds.values();
//
//  for (size_t i = 0; i < kwds_count; i++) {
//    params[args_count + i] = CPythonObject::Wrap(values[i]);
//  }
//
//  v8::MaybeLocal<v8::Value> result;
//
//  withPythonAllowThreadsGuard([&]() {
//    result = func->Call(context, self.IsEmpty() ? isolate->GetCurrentContext()->Global() : self, params.size(),
//                        params.empty() ? NULL : &params[0]);
//  });
//
//  if (result.IsEmpty())
//    CJavascriptException::ThrowIf(isolate, try_catch);
//
//  return CJSObject::Wrap(result.ToLocalChecked());
//}

pb::object CJSObjectFunction::Call2(v8::Local<v8::Object> v8_self, pb::list py_args, pb::dict py_kwargs) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
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

//py::object CJSObjectFunction::CreateWithArgs(CJSObjectFunctionPtr proto, py::tuple args, py::dict kwds) {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  if (proto->Object().IsEmpty())
//    throw CJavascriptException("Object prototype may only be an Object", PyExc_TypeError);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(proto->Object());
//
//  size_t args_count = PyTuple_Size(args.ptr());
//
//  std::vector<v8::Local<v8::Value>> params(args_count);
//
//  for (size_t i = 0; i < args_count; i++) {
//    params[i] = CPythonObject::Wrap(args[i]);
//  }
//
//  v8::Local<v8::Object> result;
//
//  withPythonAllowThreadsGuard([&]() {
//    result = func->NewInstance(context, params.size(), params.empty() ? NULL : &params[0]).ToLocalChecked();
//  });
//
//  if (result.IsEmpty())
//    CJavascriptException::ThrowIf(isolate, try_catch);
//
//  size_t kwds_count = PyMapping_Size(kwds.ptr());
//  py::list items = kwds.items();
//
//  for (size_t i = 0; i < kwds_count; i++) {
//    py::tuple item(items[i]);
//
//    py::str key(item[0]);
//    py::object value = item[1];
//
//    result->Set(context, v8u::toString(key), CPythonObject::Wrap(value)).Check();
//  }
//
//  return CJSObject::Wrap(result);
//}

pb::object CJSObjectFunction::CreateWithArgs2(CJSObjectFunctionPtr proto, pb::tuple py_args, pb::dict py_kwds) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);

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

//py::object CJSObjectFunction::ApplyJavascript(CJSObjectPtr self, py::list args, py::dict kwds) {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  return Call(self->Object(), args, kwds);
//}

pb::object CJSObjectFunction::ApplyJavascript2(CJSObjectPtr self, pb::list py_args, pb::dict py_kwds) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  return Call2(self->Object(), py_args, py_kwds);
}

//py::object CJSObjectFunction::ApplyPython(py::object self, py::list args, py::dict kwds) {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8u::checkContext(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  return Call(CPythonObject::Wrap(self)->ToObject(context).ToLocalChecked(), args, kwds);
//}

pb::object CJSObjectFunction::ApplyPython2(pb::object py_self, pb::list py_args, pb::dict py_kwds) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  auto context = v8_isolate->GetCurrentContext();

  return Call2(CPythonObject::Wrap(py_self)->ToObject(context).ToLocalChecked(), py_args, py_kwds);
}

//py::object CJSObjectFunction::Invoke(py::list args, py::dict kwds) {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  return Call(Self(), args, kwds);
//}

pb::object CJSObjectFunction::Invoke2(pb::list py_args, pb::dict py_kwds) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);

  return Call2(Self(), py_args, py_kwds);
}

const std::string CJSObjectFunction::GetName() const {
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

int CJSObjectFunction::GetLineNumber() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptLineNumber();
}

int CJSObjectFunction::GetColumnNumber() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptColumnNumber();
}

const std::string CJSObjectFunction::GetResourceName() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  return std::string(*name, name.length());
}

const std::string CJSObjectFunction::GetInferredName() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetInferredName()));

  return std::string(*name, name.length());
}

int CJSObjectFunction::GetLineOffset() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceLineOffset()->Value();
}

int CJSObjectFunction::GetColumnOffset() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceColumnOffset()->Value();
}

//py::object CJSObjectFunction::GetOwner() const {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  return CJSObject::Wrap(Self());
//}

pb::object CJSObjectFunction::GetOwner2() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  return CJSObject::Wrap(Self());
}