#include "V8Utils.h"
#include "RawUtils.h"
#include "JSException.h"

namespace v8u {

std::optional<v8::Local<v8::String>> toStringDirectly(py::handle obj) {
  if (PyUnicode_CheckExact(obj.ptr())) {
    auto raw_bytes = PyUnicode_AsUTF8String(obj.ptr());  // may be NULL
    auto py_bytes_obj = py::reinterpret_steal<py::object>(raw_bytes);
    return pythonBytesObjectToString(py_bytes_obj.ptr());
  }

  if (PyBytes_CheckExact(obj.ptr())) {
    return pythonBytesObjectToString(obj.ptr());
  }

  // please note that returning nullopt is different than
  // returning v8::Local<v8::String> with empty value inside
  // and that is different than returning v8::Local<v8::String> with empty string "" inside
  return std::nullopt;
}

v8::Local<v8::String> toString(const py::handle& py_str) {
  // first try to convert python string object directly if possible
  auto v8_str = toStringDirectly(py_str);
  if (v8_str) {
    return *v8_str;
  }

  // alternatively convert it to string representation and try again
  auto raw_computed_str = PyObject_Str(py_str.ptr());  // may be NULL
  auto py_computed_obj = py::reinterpret_steal<py::object>(raw_computed_str);
  v8_str = toStringDirectly(py_computed_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::Local<v8::String> toString(const std::wstring& str) {
  auto raw_unicode = PyUnicode_FromWideChar(str.c_str(), str.size());  // may be NULL
  auto py_obj = py::reinterpret_steal<py::object>(raw_unicode);
  auto v8_str = toStringDirectly(py_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const std::string& str) {
  return v8::String::NewFromUtf8(v8_isolate, str.c_str(), v8::NewStringType::kNormal, str.size()).ToLocalChecked();
}

v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const char* s) {
  return v8::String::NewFromUtf8(v8_isolate, s, v8::NewStringType::kNormal).ToLocalChecked();
}

v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const std::string_view& sv) {
  return v8::String::NewFromUtf8(v8_isolate, sv.data(), v8::NewStringType::kNormal, sv.size()).ToLocalChecked();
}

v8::Local<v8::String> toString(const std::string& str) {
  auto v8_isolate = v8u::getCurrentIsolate();
  return toString(v8_isolate, str);
}

v8::Local<v8::Integer> toPositiveInteger(v8::IsolatePtr v8_isolate, int i) {
  if (i >= 0) {
    return v8::Integer::New(v8_isolate, i);
  } else {
    return v8::Local<v8::Integer>();  // empty value
  }
}

v8::String::Utf8Value toUTF(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_value) {
  return v8::String::Utf8Value(v8_isolate, v8_value);
}

std::string toStdString(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_value) {
  auto v8_utf = toUTF(v8_isolate, v8_value);
  return std::string{*v8_utf, v8_utf.length()};
}

void checkContext(v8::IsolatePtr v8_isolate) {
  auto scope = withScope(v8_isolate);
  if (v8_isolate->GetCurrentContext().IsEmpty()) {
    throw CJSException(v8_isolate, "Javascript object out of context", PyExc_UnboundLocalError);
  }
}

v8::IsolatePtr getCurrentIsolate() {
  // note in debug mode there is internal check in v8::Isolate::GetCurrent(), so this fails when there is no current
  // isolate
  return v8::Isolate::GetCurrent();
}

v8::HandleScope withScope(v8::IsolatePtr v8_isolate) {
  return v8::HandleScope(v8_isolate);
}

v8::EscapableHandleScope withEscapableScope(v8::IsolatePtr v8_isolate) {
  return v8::EscapableHandleScope(v8_isolate);
}

v8::TryCatch withTryCatch(v8::IsolatePtr v8_isolate) {
  return v8::TryCatch(v8_isolate);
}

void checkTryCatch(v8::IsolatePtr v8_isolate, v8::TryCatchPtr v8_try_catch) {
  CJSException::CheckTryCatch(v8_isolate, v8_try_catch);
}

v8::IsolatePtr createIsolate() {
  v8::Isolate::CreateParams v8_create_params;
  v8_create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  auto v8_isolate = v8::Isolate::New(v8_create_params);
  assert(v8_isolate);
  return v8_isolate;
}

v8::Context::Scope withContext(v8::Local<v8::Context> v8_context) {
  return v8::Context::Scope(v8_context);
}

v8::ScriptOrigin createScriptOrigin(v8::Local<v8::Value> v8_name,
                                    v8::Local<v8::Integer> v8_line,
                                    v8::Local<v8::Integer> v8_col) {
  return v8::ScriptOrigin(v8_name, v8_line, v8_col);
}

v8::Eternal<v8::Private> createEternalPrivateAPI(v8::IsolatePtr v8_isolate, const char* name) {
  auto v8_key = v8::String::NewFromUtf8(v8_isolate, name).ToLocalChecked();
  auto v8_private_api = v8::Private::ForApi(v8_isolate, v8_key);
  return v8::Eternal<v8::Private>(v8_isolate, v8_private_api);
}

}  // namespace v8u