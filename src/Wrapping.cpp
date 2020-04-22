#include "Wrapping.h"
#include "Tracer.h"
#include "JSNull.h"
#include "JSUndefined.h"
#include "PythonDateTime.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonWrapLogger), __VA_ARGS__)

py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this) {
  TRACE("wrap v8_isolate={} v8_val={} v8_this={}", P$(v8_isolate), v8_val, v8_this);

  if (v8_val->IsFunction()) {
    // when v8_this is global, we can fall back to unbound function (later)
    auto v8_context = v8_isolate->GetCurrentContext();
    if (!v8_this->StrictEquals(v8_context->Global())) {
      // TODO: optimize this
      auto v8_fn = v8_val.As<v8::Function>();
      auto v8_bind_key = v8::String::NewFromUtf8(v8_isolate, "bind").ToLocalChecked();
      auto v8_bind_val = v8_fn->Get(v8_context, v8_bind_key).ToLocalChecked();
      assert(v8_bind_val->IsFunction());
      auto v8_bind_fn = v8_bind_val.As<v8::Function>();
      v8::Local<v8::Value> args[] = {v8_this};
      auto v8_bound_val = v8_bind_fn->Call(v8_context, v8_fn, std::size(args), args).ToLocalChecked();
      assert(v8_bound_val->IsFunction());
      auto v8_bound_fn = v8_bound_val.As<v8::Function>();
      TRACE("wrap v8_bound_fn={}", v8_bound_fn);
      // TODO: mark as function role?
      return wrap(v8_isolate, std::make_shared<CJSObject>(v8_bound_fn));
    }
  }

  return wrap(v8_isolate, v8_val);
}

py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val) {
  TRACE("wrap v8_isolate={} v8_val={}", P$(v8_isolate), v8_val);
  assert(!v8_val.IsEmpty());
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8_val->IsNull()) {
    return py::js_null();
  }
  if (v8_val->IsUndefined()) {
    return py::js_undefined();
  }
  if (v8_val->IsTrue()) {
    return py::bool_(true);
  }
  if (v8_val->IsFalse()) {
    return py::bool_(false);
  }
  if (v8_val->IsInt32()) {
    auto int32 = v8_val->Int32Value(v8_isolate->GetCurrentContext()).ToChecked();
    return py::int_(int32);
  }
  if (v8_val->IsString()) {
    auto v8_utf = v8u::toUTF(v8_isolate, v8_val.As<v8::String>());
    return py::str(*v8_utf, v8_utf.length());
  }
  if (v8_val->IsStringObject()) {
    auto v8_utf = v8u::toUTF(v8_isolate, v8_val.As<v8::StringObject>()->ValueOf());
    return py::str(*v8_utf, v8_utf.length());
  }
  if (v8_val->IsBoolean()) {
    bool val = v8_val->BooleanValue(v8_isolate);
    return py::bool_(val);
  }
  if (v8_val->IsBooleanObject()) {
    auto val = v8_val.As<v8::BooleanObject>()->BooleanValue(v8_isolate);
    return py::bool_(val);
  }
  if (v8_val->IsNumber()) {
    auto val = v8_val->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    return py::float_(val);
  }
  if (v8_val->IsNumberObject()) {
    auto val = v8_val.As<v8::NumberObject>()->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    return py::float_(val);
  }
  if (v8_val->IsDate()) {
    auto val = v8_val.As<v8::Date>()->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    auto ts = static_cast<time_t>(floor(val / 1000));
    auto t = localtime(&ts);
    auto u = (static_cast<int64_t>(floor(val))) % 1000 * 1000;
    return pythonFromDateAndTime(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, u);
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_obj = v8_val->ToObject(v8_context).ToLocalChecked();
  auto py_result = wrap(v8_isolate, v8_obj);
  TRACE("wrap => {}", py_result);
  return py_result;
}

py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj) {
  TRACE("wrap v8_isolate={} v8_obj={}", P$(v8_isolate), v8_obj);
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withScope(v8_isolate);

  py::object py_result;
  auto traced_raw_object = lookupTracedObject(v8_obj);
  if (traced_raw_object) {
    py_result = py::reinterpret_borrow<py::object>(traced_raw_object);
  } else if (v8_obj.IsEmpty()) {
    // TODO: we should not treat empty values so softly, we should throw/crash
    py_result = py::none();
  } else {
    py_result = wrap(v8_isolate, std::make_shared<CJSObject>(v8_obj));
  }

  TRACE("wrap => {}", py_result);
  return py_result;
}

py::object wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj) {
  TRACE("wrap v8_isolate={} obj={}", P$(v8_isolate), obj);
  auto py_gil = pyu::withGIL();

  if (v8u::executionTerminating(v8_isolate)) {
    return py::js_null();
  }

  auto py_result = py::cast(obj);
  TRACE("wrap => {}", py_result);
  return py_result;
}