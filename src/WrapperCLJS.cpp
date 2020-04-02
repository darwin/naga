#include "WrapperCLJS.h"
#include "PythonObject.h"
#include "Context.h"
#include "Exception.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

static auto sentinel_name = "bcljs-bridge-sentinel";

// https://stackoverflow.com/a/26221725/84283
template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
  if (size <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);  // We don't want the '\0' inside
}

std::ostream& operator<<(std::ostream& os, const CJSObject& obj);

std::ostream& operator<<(std::ostream& os, const v8::Local<v8::Object>& obj) {
  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  os << " proto=" << *v8::String::Utf8Value(isolate, obj->ObjectProtoToString(context).ToLocalChecked()) << "\n";
  os << " ctor=" << *v8::String::Utf8Value(isolate, obj->GetConstructorName()) << "\n";
  os << " str=" << *v8::String::Utf8Value(isolate, obj->ToString(context).ToLocalChecked()) << "\n";
  return os;
}

static inline void checkContext() {
  if (v8::Isolate::GetCurrent()->GetCurrentContext().IsEmpty()) {
    throw CJavascriptException("Javascript object out of context", PyExc_UnboundLocalError);
  }
}

static inline void validateBridgeResult(v8::Local<v8::Value> val, const char* fn_name) {
  if (val.IsEmpty()) {
    throw CJavascriptException(string_format("Unexpected: got empty result from bcljs.bridge.%s call", fn_name),
                               PyExc_UnboundLocalError);
  }
}

auto hint_property_name = "stpyv8hint";

void setWrapperHint(v8::Local<v8::Object> obj, uint32_t hint) {
  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  auto hint_property_str = v8::String::NewFromUtf8(isolate, hint_property_name).ToLocalChecked();
  auto private_property = v8::Private::ForApi(isolate, hint_property_str);
  auto hint_value = v8::Integer::New(isolate, hint);

  obj->SetPrivate(context, private_property, hint_value);
}

uint32_t getWrapperHint(v8::Local<v8::Object> obj) {
  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  auto hint_property_str = v8::String::NewFromUtf8(isolate, hint_property_name).ToLocalChecked();
  auto private_property = v8::Private::ForApi(isolate, hint_property_str);
  auto res_val = obj->GetPrivate(context, private_property).ToLocalChecked();

  if (res_val.IsEmpty()) {
    return kWrapperHintNone;
  }

  if (!res_val->IsNumber()) {
    return kWrapperHintNone;
  }

  return res_val->Uint32Value(context).ToChecked();
}

bool isCLJSType(v8::Local<v8::Object> obj) {
  // early rejection
  if (obj.IsEmpty()) {
    return false;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  auto ctor_key = v8::String::NewFromUtf8(isolate, "constructor").ToLocalChecked();
  auto ctor_val = obj->Get(context, ctor_key).ToLocalChecked();

  if (ctor_val.IsEmpty() || !ctor_val->IsObject()) {
    return false;
  }

  auto ctor_obj = v8::Local<v8::Object>::Cast(ctor_val);
  auto cljs_key = v8::String::NewFromUtf8(isolate, "cljs$lang$type").ToLocalChecked();
  auto cljs_val = ctor_obj->Get(context, cljs_key).ToLocalChecked();

  return !(cljs_val.IsEmpty() || !cljs_val->IsBoolean());

  // note:
  // we should cast cljs_val to object and check cljs_obj->BooleanValue()
  // but we assume existence of property means true value
}

void exposeCLJSTypes() {
  py::class_<CCLJSType, py::bases<CJSObject>, boost::noncopyable>("CLJSType", py::no_init)
      .def("__str__", &CCLJSType::Str)
      .def("__repr__", &CCLJSType::Repr)
      .def("__len__", &CCLJSType::Length)
      .def("__getitem__", &CCLJSType::GetItem)
      .def("__getattr__", &CCLJSType::GetAttr)
      //.def("__contains__", &CCLJSType::Contains)
      .def("__iter__", &CCLJSType::Iter);

  py::class_<CCLJSIIterableIterator, py::bases<CJSObject>, boost::noncopyable>("CLJSIIterableIterator", py::no_init)
      .def("__next__", &CCLJSIIterableIterator::Next)
      .def("__iter__", boost::python::objects::identity_function());
}

v8::Local<v8::Function> lookup_bridge_fn(const char* name) {
  // TODO: caching? review performance

  auto isolate = v8::Isolate::GetCurrent();
  auto context = isolate->GetCurrentContext();
  auto global_obj = isolate->GetCurrentContext()->Global();

  auto bcljs_key = v8::String::NewFromUtf8(isolate, "bcljs").ToLocalChecked();
  auto bcljs_val = global_obj->Get(context, bcljs_key).ToLocalChecked();

  if (bcljs_val.IsEmpty()) {
    auto msg = "Unable to retrieve bcljs in global js context";
    throw CJavascriptException(msg, PyExc_UnboundLocalError);
  }

  if (!bcljs_val->IsObject()) {
    auto msg = "Unexpected: bcljs in global js context in not a js object";
    throw CJavascriptException(msg, PyExc_UnboundLocalError);
  }

  auto bcljs_obj = v8::Local<v8::Object>::Cast(bcljs_val);

  auto bridge_key = v8::String::NewFromUtf8(isolate, "bridge").ToLocalChecked();
  auto bridge_val = bcljs_obj->Get(context, bridge_key).ToLocalChecked();

  if (bridge_val.IsEmpty() || bridge_val->IsNullOrUndefined()) {
    auto msg = "Unable to retrieve bcljs.bridge in global js context";
    throw CJavascriptException(msg, PyExc_UnboundLocalError);
  }

  if (!bridge_val->IsObject()) {
    auto msg = "Unexpected: bcljs.bridge in global js context in not a js object";
    throw CJavascriptException(msg, PyExc_TypeError);
  }

  auto bridge_obj = v8::Local<v8::Object>::Cast(bridge_val);

  auto fn_key = v8::String::NewFromUtf8(isolate, name).ToLocalChecked();
  auto fn_val = bridge_obj->Get(context, fn_key).ToLocalChecked();

  if (fn_val.IsEmpty() || fn_val->IsNullOrUndefined()) {
    auto msg = string_format("Unable to retrieve bcljs.bridge.%s", name);
    throw CJavascriptException(msg, PyExc_UnboundLocalError);
  }

  if (!fn_val->IsFunction()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s must be a js function", name);
    throw CJavascriptException(msg, PyExc_TypeError);
  }

  auto fn_obj = v8::Local<v8::Function>::Cast(fn_val);
  return fn_obj;
}

v8::Local<v8::Value> call_bridge(v8::Isolate* isolate,
                                 const char* name,
                                 v8::Local<v8::Object> self,
                                 std::vector<v8::Local<v8::Value>> params) {
  auto context = isolate->GetCurrentContext();
  v8::TryCatch try_catch(isolate);
  auto func = lookup_bridge_fn(name);

  v8::MaybeLocal<v8::Value> result;
  result = func->Call(context, self, params.size(), params.empty() ? nullptr : &params[0]);

  if (result.IsEmpty()) {
    CJavascriptException::ThrowIf(isolate, try_catch);
  }

  return result.ToLocalChecked();
}

size_t CCLJSType::Length() {
  checkContext();

  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  auto context = isolate->GetCurrentContext();

  auto params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "len";
  auto res_val = call_bridge(isolate, fn_name, Object(), params);

  validateBridgeResult(res_val, fn_name);

  if (!res_val->IsNumber()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s expected to return a number", fn_name);
    throw CJavascriptException(msg, PyExc_TypeError);
  }

  auto val = res_val->Uint32Value(context);
  return val.ToChecked();
}

py::object CCLJSType::Str() {
  checkContext();

  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  auto params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "str";
  auto res_val = call_bridge(isolate, fn_name, Object(), params);

  validateBridgeResult(res_val, fn_name);

  if (!res_val->IsString()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s expected to return a string", fn_name);
    throw CJavascriptException(msg, PyExc_TypeError);
  }

  auto str = *v8::String::Utf8Value(isolate, res_val);
  auto py_str = PyUnicode_FromString(str);
  return py::object(py::handle<>(py_str));
}

py::object CCLJSType::Repr() {
  checkContext();

  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  auto params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "repr";
  auto res_val = call_bridge(isolate, fn_name, Object(), params);

  validateBridgeResult(res_val, fn_name);

  if (!res_val->IsString()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s expected to return a string", fn_name);
    throw CJavascriptException(msg, PyExc_TypeError);
  }

  auto str = *v8::String::Utf8Value(isolate, res_val);
  auto py_str = PyUnicode_FromString(str);
  return py::object(py::handle<>(py_str));
}

bool is_sentinel(v8::Local<v8::Value> val) {
  if (!val->IsSymbol()) {
    return false;
  }

  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  auto res_sym = v8::Local<v8::Symbol>::Cast(val);
  auto sentinel_name_key = v8::String::NewFromUtf8(isolate, sentinel_name).ToLocalChecked();
  auto sentinel = res_sym->For(isolate, sentinel_name_key);
  return res_sym->SameValue(sentinel);
}

py::object CCLJSType::GetItemIndex(const py::object& py_index) {
  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  uint32_t idx = PyLong_AsUnsignedLong(py_index.ptr());

  auto params = std::vector<v8::Local<v8::Value>>{v8::Uint32::New(isolate, idx)};
  auto fn_name = "get_item_index";
  auto res_val = call_bridge(isolate, fn_name, Object(), params);

  validateBridgeResult(res_val, fn_name);

  if (is_sentinel(res_val)) {
    throw CJavascriptException("CLJSType index out of bounds", ::PyExc_IndexError);
  }
  return CJSObject::Wrap(res_val, Object());
}

py::object CCLJSType::GetItemSlice(const py::object& py_slice) {
  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  Py_ssize_t length = Length();
  Py_ssize_t start, stop, step;
  auto slice = py_slice.ptr();

  if (PySlice_Unpack(slice, &start, &stop, &step) < 0) {
    CPythonObject::ThrowIf(isolate);
  }
  PySlice_AdjustIndices(length, &start, &stop, step);

  auto start_int = v8::Integer::New(isolate, start);
  auto stop_int = v8::Integer::New(isolate, stop);
  auto step_int = v8::Integer::New(isolate, step);

  auto params = std::vector<v8::Local<v8::Value>>{start_int, stop_int, step_int};
  auto fn_name = "get_item_slice";
  auto res_val = call_bridge(isolate, fn_name, Object(), params);

  validateBridgeResult(res_val, fn_name);

  if (!res_val->IsArray()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s must return a js array", fn_name);
    throw CJavascriptException(msg, PyExc_TypeError);
  }

  auto res_arr = v8::Local<v8::Array>::Cast(res_val);
  auto res_len = res_arr->Length();

  py::list py_res;

  auto context = isolate->GetCurrentContext();
  for (auto i = 0U; i < res_len; i++) {
    auto item = res_arr->Get(context, v8::Integer::New(isolate, i)).ToLocalChecked();

    if (item.IsEmpty()) {
      auto msg = string_format("Got empty value at index %d when processing slice", i);
      throw CJavascriptException(msg, PyExc_UnboundLocalError);
    }

    py_res.append(CJSObject::Wrap(item, Object()));
  }

  return std::move(py_res);
}

py::object CCLJSType::GetItemString(const py::object& py_string) {
  assert(PyUnicode_Check(py_string.ptr()));

  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();

  auto key_val = ToString(py_string);

  // JS object lookup
  if (Object()->Has(context, key_val).FromMaybe(false)) {
    auto js_val = Object()->Get(context, key_val).ToLocalChecked();
    if (js_val.IsEmpty()) {
      auto msg = string_format("Unexpected: got empty result when accessing js property '%s'",
                               *v8::String::Utf8Value(isolate, js_val));
      throw CJavascriptException(msg, PyExc_UnboundLocalError);
    }
    return CJSObject::Wrap(js_val, Object());
  }

  // CLJS lookup
  auto params = std::vector<v8::Local<v8::Value>>{key_val};
  auto fn_name = "get_item_string";
  auto res_val = call_bridge(isolate, fn_name, Object(), params);

  validateBridgeResult(res_val, fn_name);

  if (is_sentinel(res_val)) {
    return py::object();
  }
  return CJSObject::Wrap(res_val, Object());
}

py::object CCLJSType::GetItem(const py::object& py_key) {
  checkContext();

  if (PyLong_Check(py_key.ptr()) != 0) {
    return GetItemIndex(py_key);
  }

  if (PySlice_Check(py_key.ptr()) != 0) {
    return GetItemSlice(py_key);
  }

  throw CJavascriptException("indices must be integers or slices", ::PyExc_TypeError);
}

py::object CCLJSType::GetAttr(const py::object& py_key) {
  checkContext();

  if (PyUnicode_Check(py_key.ptr()) != 0) {
    return GetItemString(py_key);
  }

  throw CJavascriptException("attr names must be strings", ::PyExc_TypeError);
}

py::object CCLJSType::Iter() {
  checkContext();

  auto isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  auto params = std::vector<v8::Local<v8::Value>>();
  auto fn_name = "iter";
  auto res_val = call_bridge(isolate, fn_name, Object(), params);

  validateBridgeResult(res_val, fn_name);

  if (!res_val->IsObject()) {
    auto msg = string_format("Unexpected: bcljs.bridge.%s expected to return a js object", fn_name);
    throw CJavascriptException(msg, PyExc_TypeError);
  }

  auto iterator_obj = v8::Local<v8::Object>::Cast(res_val);
  setWrapperHint(iterator_obj, kWrapperHintCCLJSIIterableIterator);
  auto it = new CCLJSIIterableIterator(iterator_obj);
  return CCLJSIIterableIterator::Wrap(it);
}

py::object CCLJSIIterableIterator::Next() {
  // TODO: implement
  return py::object();
}

// https://wiki.python.org/moin/boost.python/iterator
py::object CCLJSIIterableIterator::Iter(const py::object& py_iter) {
  // pass through
  return py_iter;
}

#pragma clang diagnostic pop