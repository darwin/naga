#include "Wrapping.h"
#include "Tracer.h"
#include "JSNull.h"
#include "JSUndefined.h"
#include "PythonDateTime.h"
#include "PythonObject.h"
#include "JSException.h"
#include "JSObject.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kWrappingLogger), __VA_ARGS__)

py::object wrap(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this) {
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

py::object wrap(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_val) {
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
  TRACE("=> {}", py_result);
  return py_result;
}

py::object wrap(v8::IsolatePtr v8_isolate, v8::Local<v8::Object> v8_obj) {
  TRACE("wrap v8_isolate={} v8_obj={}", P$(v8_isolate), v8_obj);
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8_obj.IsEmpty()) {
    throw CJSException(v8_isolate, "Unexpected empty V8 object handle.");
  }

  py::object py_result;
  auto traced_raw_object = lookupTracedObject(v8_obj);
  if (traced_raw_object) {
    py_result = py::reinterpret_borrow<py::object>(traced_raw_object);
  } else {
    py_result = wrap(v8_isolate, std::make_shared<CJSObject>(v8_obj));
  }

  TRACE("=> {}", py_result);
  return py_result;
}

py::object wrap(v8::IsolatePtr v8_isolate, const CJSObjectPtr& obj) {
  TRACE("wrap v8_isolate={} obj={}", P$(v8_isolate), obj);
  auto py_gil = pyu::withGIL();

  auto py_result = py::cast(obj);
  TRACE("=> {}", py_result);
  return py_result;
}

static v8::Local<v8::Value> wrapWithTracing(v8::IsolatePtr v8_isolate, py::handle py_handle) {
  TRACE("wrapWithTracing v8_isolate={} py_handle={}", P$(v8_isolate), py_handle);

  auto v8_wrapper = lookupTracedWrapper(v8_isolate, py_handle.ptr());
  if (!v8_wrapper.IsEmpty()) {
    // this is fast path, we've already seen this python object before and we have cached wrapper for it
    return v8_wrapper;
  } else {
    // this is first time we see this object, let's create a new wrapper for it
    auto v8_context = v8_isolate->GetCurrentContext();
    auto v8_wrapper_template = CPythonObject::GetOrCreateCachedJSWrapperTemplate(v8_isolate);
    auto v8_new_wrapper = v8_wrapper_template->NewInstance(v8_context).ToLocalChecked();
    assert(!v8_new_wrapper.IsEmpty());

    // start tracing the wrapper for future lookups, see Tracer.h for explanation
    traceWrapper(py_handle.ptr(), v8_new_wrapper);

    return v8_new_wrapper;
  }
}

static v8::Local<v8::Value> wrapInternal(v8::IsolatePtr v8_isolate, py::handle py_handle) {
  TRACE("wrapInternal v8_isolate={} py_handle={}", P$(v8_isolate), py_handle);
  if (py::isinstance<py::js_null>(py_handle)) {
    return v8::Null(v8_isolate);
  }
  if (py::isinstance<py::js_undefined>(py_handle)) {
    return v8::Undefined(v8_isolate);
  }
  if (py::isinstance<py::bool_>(py_handle)) {
    auto py_bool = py::cast<py::bool_>(py_handle);
    if (py_bool) {
      return v8::True(v8_isolate);
    } else {
      return v8::False(v8_isolate);
    }
  }
  if (py::isinstance<py::exact_int>(py_handle)) {
    auto py_int = py::cast<py::exact_int>(py_handle);
    return v8::Integer::New(v8_isolate, py_int);
  }
  if (py::isinstance<py::exact_float>(py_handle)) {
    auto py_float = py::cast<py::exact_float>(py_handle);
    return v8::Number::New(v8_isolate, py_float);
  }
  if (py::isinstance<py::exact_str>(py_handle)) {
    return v8u::toString(py_handle);
  }
  if (isExactDateTime(py_handle) || isExactDate(py_handle)) {
    tm ts = {0};
    int ms = 0;
    getPythonDateTime(py_handle, ts, ms);
    auto v8_context = v8_isolate->GetCurrentContext();
    auto time = (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000;
    return v8::Date::New(v8_context, time).ToLocalChecked();
  }
  if (isExactTime(py_handle)) {
    tm ts = {0};
    int ms = 0;
    getPythonTime(py_handle, ts, ms);
    auto v8_context = v8_isolate->GetCurrentContext();
    auto time = (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000;
    return v8::Date::New(v8_context, time).ToLocalChecked();
  }
  if (py::isinstance<CJSObject>(py_handle)) {
    auto object = py::cast<CJSObjectPtr>(py_handle);
    assert(object.get());
    return object->ToV8(v8_isolate);
  }

  // fall back to synthesizing custom wrapper on the fly
  return wrapWithTracing(v8_isolate, py_handle);
}

v8::Local<v8::Value> wrap(const py::handle& py_handle) {
  TRACE("wrap py_handle={}", py_handle);
  auto v8_isolate = v8u::getCurrentIsolate();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withEscapableScope(v8_isolate);
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);
  auto py_gil = pyu::withGIL();
  auto v8_result = wrapInternal(v8_isolate, py_handle);
  return v8_scope.Escape(v8_result);
}
