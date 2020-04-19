#include "JSObjectCLJSImpl.h"
#include "PythonObject.h"
#include "JSException.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

static const auto sentinel_name = "bcljs-bridge-sentinel";

static inline void validateBridgeResult(v8::Local<v8::Value> v8_val, const char* fn_name) {
  if (v8_val.IsEmpty()) {
    throw CJSException(string_format("Unexpected: got empty result from bcljs.bridge.%s call", fn_name),
                       PyExc_UnboundLocalError);
  }
}

static v8::Local<v8::Function> lookupBridgeFn(const char* name) {
  // TODO: caching? review performance

  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_global = v8_isolate->GetCurrentContext()->Global();

  auto v8_bcljs_key = v8::String::NewFromUtf8(v8_isolate, "bcljs").ToLocalChecked();
  auto v8_bcljs_val = v8_global->Get(v8_context, v8_bcljs_key).ToLocalChecked();

  if (v8_bcljs_val.IsEmpty()) {
    auto msg = "Unable to retrieve bcljs in global js context";
    throw CJSException(msg, PyExc_UnboundLocalError);
  }

  if (!v8_bcljs_val->IsObject()) {
    auto msg = "Unexpected: bcljs in global js context in not a js object";
    throw CJSException(msg, PyExc_UnboundLocalError);
  }

  auto v8_bcljs_obj = v8::Local<v8::Object>::Cast(v8_bcljs_val);

  auto v8_bridge_key = v8::String::NewFromUtf8(v8_isolate, "bridge").ToLocalChecked();
  auto v8_bridge_val = v8_bcljs_obj->Get(v8_context, v8_bridge_key).ToLocalChecked();

  if (v8_bridge_val.IsEmpty() || v8_bridge_val->IsNullOrUndefined()) {
    auto msg = "Unable to retrieve bcljs.bridge in global js context";
    throw CJSException(msg, PyExc_UnboundLocalError);
  }

  if (!v8_bridge_val->IsObject()) {
    auto msg = "Unexpected: bcljs.bridge in global js context in not a js object";
    throw CJSException(msg, PyExc_TypeError);
  }

  auto v8_bridge_obj = v8::Local<v8::Object>::Cast(v8_bridge_val);

  auto v8_fn_key = v8::String::NewFromUtf8(v8_isolate, name).ToLocalChecked();
  auto v8_fn_val = v8_bridge_obj->Get(v8_context, v8_fn_key).ToLocalChecked();

  if (v8_fn_val.IsEmpty() || v8_fn_val->IsNullOrUndefined()) {
    auto msg = string_format("Unable to retrieve bcljs.bridge.%s", name);
    throw CJSException(msg, PyExc_UnboundLocalError);
  }

  if (!v8_fn_val->IsFunction()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s must be a js function", name);
    throw CJSException(msg, PyExc_TypeError);
  }

  return v8_fn_val.As<v8::Function>();
}

static v8::Local<v8::Value> callBridge(v8::IsolateRef v8_isolate,
                                       const char* name,
                                       v8::Local<v8::Object> v8_self,
                                       std::vector<v8::Local<v8::Value>> v8_params) {
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);
  auto v8_fn = lookupBridgeFn(name);

  auto v8_result = v8_fn->Call(v8_context, v8_self, v8_params.size(), v8_params.data());
  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return v8_result.ToLocalChecked();
}

static bool isSentinel(v8::Local<v8::Value> v8_val) {
  if (!v8_val->IsSymbol()) {
    return false;
  }

  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_res_sym = v8::Local<v8::Symbol>::Cast(v8_val);
  auto v8_sentinel_name_key = v8::String::NewFromUtf8(v8_isolate, sentinel_name).ToLocalChecked();
  auto v8_sentinel = v8_res_sym->For(v8_isolate, v8_sentinel_name_key);
  return v8_res_sym->SameValue(v8_sentinel);
}

size_t CJSObjectCLJSImpl::CLJSLength() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  auto v8_params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "len";
  auto v8_result = callBridge(v8_isolate, fn_name, m_base.Object(), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsNumber()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s expected to return a number", fn_name);
    throw CJSException(msg, PyExc_TypeError);
  }

  auto val = v8_result->Uint32Value(v8_context);
  return val.ToChecked();
}

py::object CJSObjectCLJSImpl::CLJSStr() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "str";
  auto v8_result = callBridge(v8_isolate, fn_name, m_base.Object(), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsString()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s expected to return a string", fn_name);
    throw CJSException(msg, PyExc_TypeError);
  }

  auto v8_utf = v8::String::Utf8Value(v8_isolate, v8_result);
  auto raw_str = PyUnicode_FromString(*v8_utf);
  return py::cast<py::object>(raw_str);
}

py::object CJSObjectCLJSImpl::CLJSRepr() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "repr";
  auto v8_result = callBridge(v8_isolate, fn_name, m_base.Object(), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsString()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s expected to return a string", fn_name);
    throw CJSException(msg, PyExc_TypeError);
  }

  auto v8_utf = v8::String::Utf8Value(v8_isolate, v8_result);
  auto raw_str = PyUnicode_FromString(*v8_utf);
  return py::cast<py::object>(raw_str);
}

py::object CJSObjectCLJSImpl::CLJSGetItemIndex(const py::object& py_index) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto idx = PyLong_AsUnsignedLong(py_index.ptr());

  auto v8_idx = v8::Uint32::New(v8_isolate, idx);
  auto v8_params = std::vector<v8::Local<v8::Value>>{v8_idx};
  auto fn_name = "get_item_index";
  auto v8_result = callBridge(v8_isolate, fn_name, m_base.Object(), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (isSentinel(v8_result)) {
    throw CJSException("CLJSType index out of bounds", PyExc_IndexError);
  }
  return CJSObject::Wrap(v8_isolate, v8_result, m_base.Object());
}

py::object CJSObjectCLJSImpl::CLJSGetItemSlice(const py::object& py_slice) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  Py_ssize_t length = CLJSLength();
  Py_ssize_t start;
  Py_ssize_t stop;
  Py_ssize_t step;

  if (PySlice_Unpack(py_slice.ptr(), &start, &stop, &step) < 0) {
    CPythonObject::ThrowIf(v8_isolate);
  }
  PySlice_AdjustIndices(length, &start, &stop, step);

  auto v8_start = v8::Integer::New(v8_isolate, start);
  auto v8_stop = v8::Integer::New(v8_isolate, stop);
  auto v8_step = v8::Integer::New(v8_isolate, step);

  auto v8_params = std::vector<v8::Local<v8::Value>>{v8_start, v8_stop, v8_step};
  auto fn_name = "get_item_slice";
  auto v8_result = callBridge(v8_isolate, fn_name, m_base.Object(), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (!v8_result->IsArray()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s must return a js array", fn_name);
    throw CJSException(msg, PyExc_TypeError);
  }

  auto v8_result_arr = v8_result.As<v8::Array>();
  auto result_arr_len = v8_result_arr->Length();

  py::list py_result;

  auto v8_context = v8_isolate->GetCurrentContext();
  for (auto i = 0U; i < result_arr_len; i++) {
    auto v8_i = v8::Integer::New(v8_isolate, i);
    auto v8_item = v8_result_arr->Get(v8_context, v8_i).ToLocalChecked();

    if (v8_item.IsEmpty()) {
      auto msg = string_format("Got empty value at index %d when processing slice", i);
      throw CJSException(msg, PyExc_UnboundLocalError);
    }

    auto py_item = CJSObject::Wrap(v8_isolate, v8_item, m_base.Object());
    py_result.append(py_item);
  }

  return std::move(py_result);
}

py::object CJSObjectCLJSImpl::CLJSGetItemString(const py::object& py_str) const {
  assert(PyUnicode_Check(py_str.ptr()));

  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_str = v8u::toString(py_str);

  // JS object lookup
  if (m_base.Object()->Has(v8_context, v8_str).FromMaybe(false)) {
    auto v8_val = m_base.Object()->Get(v8_context, v8_str).ToLocalChecked();
    if (v8_val.IsEmpty()) {
      auto v8_utf = v8::String::Utf8Value(v8_isolate, v8_val);
      auto msg = string_format("Unexpected: got empty result when accessing js property '%s'", *v8_utf);
      throw CJSException(msg, PyExc_UnboundLocalError);
    }
    return CJSObject::Wrap(v8_isolate, v8_val, m_base.Object());
  }

  // CLJS lookup
  auto v8_params = std::vector<v8::Local<v8::Value>>{v8_str};
  auto fn_name = "get_item_string";
  auto v8_result = callBridge(v8_isolate, fn_name, m_base.Object(), v8_params);

  validateBridgeResult(v8_result, fn_name);

  if (isSentinel(v8_result)) {
    return py::none();
  }
  return CJSObject::Wrap(v8_isolate, v8_result, m_base.Object());
}

py::object CJSObjectCLJSImpl::CLJSGetItem(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);

  if (PyLong_Check(py_key.ptr()) != 0) {
    return CLJSGetItemIndex(py_key);
  }

  if (PySlice_Check(py_key.ptr()) != 0) {
    return CLJSGetItemSlice(py_key);
  }

  throw CJSException("indices must be integers or slices", PyExc_TypeError);
}

py::object CJSObjectCLJSImpl::CLJSGetAttr(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);

  if (PyUnicode_Check(py_key.ptr()) != 0) {
    return CLJSGetItemString(py_key);
  }

  throw CJSException("attr names must be strings", PyExc_TypeError);
}

#pragma clang diagnostic pop