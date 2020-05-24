#include "JSObjectCLJSImpl.h"
#include "PythonObject.h"
#include "JSException.h"
#include "Wrapping.h"
#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectCLJSImplLogger), __VA_ARGS__)

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

static const auto sentinel_name = "bcljs-bridge-sentinel";

static inline void validateBridgeResult(v8::Local<v8::Value> v8_val, const char* fn_name) {
  if (v8_val.IsEmpty()) {
    throw JSException(fmt::format("Unexpected: got empty result from bcljs.bridge.{} call", fn_name),
                      PyExc_UnboundLocalError);
  }
}

static v8::Local<v8::Function> lookupBridgeFn(const char* name) {
  // TODO: caching? review performance

  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_global = v8_context->Global();

  auto v8_bcljs_key = v8::String::NewFromUtf8(v8_isolate, "bcljs").ToLocalChecked();
  auto v8_bcljs_val = v8_global->Get(v8_context, v8_bcljs_key).ToLocalChecked();

  if (v8_bcljs_val.IsEmpty()) {
    auto msg = "Unable to retrieve bcljs in global js context";
    throw JSException(msg, PyExc_UnboundLocalError);
  }

  if (!v8_bcljs_val->IsObject()) {
    auto msg = "Unexpected: bcljs in global js context in not a js object";
    throw JSException(msg, PyExc_UnboundLocalError);
  }

  auto v8_bcljs_obj = v8::Local<v8::Object>::Cast(v8_bcljs_val);

  auto v8_bridge_key = v8::String::NewFromUtf8(v8_isolate, "bridge").ToLocalChecked();
  auto v8_bridge_val = v8_bcljs_obj->Get(v8_context, v8_bridge_key).ToLocalChecked();

  if (v8_bridge_val.IsEmpty() || v8_bridge_val->IsNullOrUndefined()) {
    auto msg = "Unable to retrieve bcljs.bridge in global js context";
    throw JSException(msg, PyExc_UnboundLocalError);
  }

  if (!v8_bridge_val->IsObject()) {
    auto msg = "Unexpected: bcljs.bridge in global js context in not a js object";
    throw JSException(msg, PyExc_TypeError);
  }

  auto v8_bridge_obj = v8::Local<v8::Object>::Cast(v8_bridge_val);

  auto v8_fn_key = v8::String::NewFromUtf8(v8_isolate, name).ToLocalChecked();
  auto v8_fn_val = v8_bridge_obj->Get(v8_context, v8_fn_key).ToLocalChecked();

  if (v8_fn_val.IsEmpty() || v8_fn_val->IsNullOrUndefined()) {
    auto msg = fmt::format("Unable to retrieve bcljs.bridge.{}", name);
    throw JSException(msg, PyExc_UnboundLocalError);
  }

  if (!v8_fn_val->IsFunction()) {
    auto msg = fmt::format("Unexpected: bcljs.bridge.{} must be a js function", name);
    throw JSException(msg, PyExc_TypeError);
  }

  return v8_fn_val.As<v8::Function>();
}

static v8::Local<v8::Value> callBridge(v8x::LockedIsolatePtr& v8_isolate,
                                       const char* name,
                                       v8::Local<v8::Object> v8_self,
                                       std::vector<v8::Local<v8::Value>> v8_params) {
  TRACE("callBridge v8_isolate={} name={}", P$(v8_isolate), name);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_fn = lookupBridgeFn(name);
  auto v8_result = v8_fn->Call(v8_context, v8_self, v8_params.size(), v8_params.data());
  return v8_result.ToLocalChecked();
}

static bool isSentinel(v8::Local<v8::Value> v8_val) {
  if (!v8_val->IsSymbol()) {
    return false;
  }

  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_res_sym = v8::Local<v8::Symbol>::Cast(v8_val);
  auto v8_sentinel_name_key = v8::String::NewFromUtf8(v8_isolate, sentinel_name).ToLocalChecked();
  auto v8_sentinel = v8_res_sym->For(v8_isolate, v8_sentinel_name_key);
  return v8_res_sym->SameValue(v8_sentinel);
}

py::ssize_t JSObjectCLJSLength(const JSObject& self) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);

  auto v8_params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "len";
  auto v8_result = callBridge(v8_isolate, fn_name, self.ToV8(v8_isolate), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsNumber()) {
    auto msg = fmt::format("Unexpected: bcljs.bridge.{} expected to return a number", fn_name);
    throw JSException(msg, PyExc_TypeError);
  }

  auto result = v8_result->Uint32Value(v8_context).ToChecked();
  TRACE("JSObjectCLJSLength {} => {}", SELF, result);
  return result;
}

py::str JSObjectCLJSStr(const JSObject& self) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto v8_params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "str";
  auto v8_result = callBridge(v8_isolate, fn_name, self.ToV8(v8_isolate), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsString()) {
    auto msg = fmt::format("Unexpected: bcljs.bridge.{} expected to return a string", fn_name);
    throw JSException(msg, PyExc_TypeError);
  }

  auto v8_utf = v8::String::Utf8Value(v8_isolate, v8_result);
  py::str py_result(*v8_utf);
  TRACE("JSObjectCLJSStr {} => {}", SELF, py_result);
  return py_result;
}

py::str JSObjectCLJSRepr(const JSObject& self) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto v8_params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "repr";
  auto v8_result = callBridge(v8_isolate, fn_name, self.ToV8(v8_isolate), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsString()) {
    auto msg = fmt::format("Unexpected: bcljs.bridge.{} expected to return a string", fn_name);
    throw JSException(msg, PyExc_TypeError);
  }

  auto v8_utf = v8::String::Utf8Value(v8_isolate, v8_result);
  py::str py_result(*v8_utf);
  TRACE("JSObjectCLJSRepr {} => {}", SELF, py_result);
  return py_result;
}

py::object JSObjectCLJSGetItemIndex(const JSObject& self, const py::object& py_index) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto idx = PyLong_AsUnsignedLong(py_index.ptr());

  auto v8_idx = v8::Uint32::New(v8_isolate, idx);
  auto v8_params = std::vector<v8::Local<v8::Value>>{v8_idx};
  auto fn_name = "get_item_index";
  auto v8_result = callBridge(v8_isolate, fn_name, self.ToV8(v8_isolate), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (isSentinel(v8_result)) {
    throw JSException("CLJSType index out of bounds", PyExc_IndexError);
  }
  auto py_result = wrap(v8_isolate, v8_result, self.ToV8(v8_isolate));
  TRACE("JSObjectCLJSGetItemIndex {} py_index={} => {}", SELF, py_index, py_result);
  return py_result;
}

py::object JSObjectCLJSGetItemSlice(const JSObject& self, const py::object& py_slice) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  Py_ssize_t length = JSObjectCLJSLength(self);
  Py_ssize_t start;
  Py_ssize_t stop;
  Py_ssize_t step;

  if (PySlice_Unpack(py_slice.ptr(), &start, &stop, &step) < 0) {
    PythonObject::ThrowJSException(v8_isolate);
  }
  PySlice_AdjustIndices(length, &start, &stop, step);

  auto v8_start = v8::Integer::New(v8_isolate, start);
  auto v8_stop = v8::Integer::New(v8_isolate, stop);
  auto v8_step = v8::Integer::New(v8_isolate, step);

  auto v8_params = std::vector<v8::Local<v8::Value>>{v8_start, v8_stop, v8_step};
  auto fn_name = "get_item_slice";
  auto v8_result = callBridge(v8_isolate, fn_name, self.ToV8(v8_isolate), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsArray()) {
    auto msg = fmt::format("Unexpected: bcljs.bridge.{} must return a js array", fn_name);
    throw JSException(msg, PyExc_TypeError);
  }

  auto v8_result_arr = v8_result.As<v8::Array>();
  auto result_arr_len = v8_result_arr->Length();

  py::list py_result;

  auto v8_context = v8x::getCurrentContext(v8_isolate);
  for (auto i = 0U; i < result_arr_len; i++) {
    auto v8_i = v8::Integer::New(v8_isolate, i);
    auto v8_item = v8_result_arr->Get(v8_context, v8_i).ToLocalChecked();

    if (v8_item.IsEmpty()) {
      auto msg = fmt::format("Got empty value at index {} when processing slice", i);
      throw JSException(msg, PyExc_UnboundLocalError);
    }

    auto py_item = wrap(v8_isolate, v8_item, self.ToV8(v8_isolate));
    py_result.append(py_item);
  }

  TRACE("JSObjectCLJSGetItemSlice {} py_slice={} => {}", SELF, py_slice, py_result);
  return std::move(py_result);
}

py::object JSObjectCLJSGetItemString(const JSObject& self, const py::object& py_str) {
  assert(PyUnicode_Check(py_str.ptr()));

  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_str = v8x::toString(v8_isolate, py_str);

  // JS object lookup
  if (self.ToV8(v8_isolate)->Has(v8_context, v8_str).FromMaybe(false)) {
    auto v8_val = self.ToV8(v8_isolate)->Get(v8_context, v8_str).ToLocalChecked();
    if (v8_val.IsEmpty()) {
      auto v8_utf = v8::String::Utf8Value(v8_isolate, v8_val);
      auto msg = fmt::format("Unexpected: got empty result when accessing js property '{}'", *v8_utf);
      throw JSException(msg, PyExc_UnboundLocalError);
    }
    return wrap(v8_isolate, v8_val, self.ToV8(v8_isolate));
  }

  // CLJS lookup
  auto v8_params = std::vector<v8::Local<v8::Value>>{v8_str};
  auto fn_name = "get_item_string";
  auto v8_result = callBridge(v8_isolate, fn_name, self.ToV8(v8_isolate), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (isSentinel(v8_result)) {
    return py::none();
  }
  auto py_result = wrap(v8_isolate, v8_result, self.ToV8(v8_isolate));
  TRACE("JSObjectCLJSGetItemString {} py_str={} => {}", SELF, py_str, py_result);
  return py_result;
}

py::object JSObjectCLJSGetItem(const JSObject& self, const py::object& py_key) {
  TRACE("JSObjectCLJSGetItem {} py_key={}", SELF, py_key);

  if (PyLong_Check(py_key.ptr()) != 0) {
    return JSObjectCLJSGetItemIndex(self, py_key);
  }

  if (PySlice_Check(py_key.ptr()) != 0) {
    return JSObjectCLJSGetItemSlice(self, py_key);
  }

  throw JSException("indices must be integers or slices", PyExc_TypeError);
}

py::object JSObjectCLJSGetAttr(const JSObject& self, const py::object& py_key) {
  TRACE("JSObjectCLJSGetAttr {} py_key={}", SELF, py_key);

  if (PyUnicode_Check(py_key.ptr()) != 0) {
    return JSObjectCLJSGetItemString(self, py_key);
  }

  throw JSException("attr names must be strings", PyExc_TypeError);
}

#pragma clang diagnostic pop