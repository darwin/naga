#include "V8XUtils.h"
#include "JSException.h"
#include "JSIsolate.h"

namespace v8x {

v8::Local<v8::String> pythonBytesObjectToString(LockedIsolatePtr& v8_isolate, PyObject* raw_bytes_obj) {
  if (!raw_bytes_obj) {
    return v8::Local<v8::String>();
  }

  assert(PyBytes_CheckExact(raw_bytes_obj));
  auto s = PyBytes_AS_STRING(raw_bytes_obj);
  assert(s);
  auto sz = PyBytes_GET_SIZE(raw_bytes_obj);
  assert(sz >= 0);
  return v8::String::NewFromUtf8(v8_isolate, s, v8::NewStringType::kNormal, sz).ToLocalChecked();
}

std::optional<v8::Local<v8::String>> toStringDirectly(LockedIsolatePtr& v8_isolate, py::handle obj) {
  if (PyUnicode_CheckExact(obj.ptr())) {
    auto raw_bytes = PyUnicode_AsUTF8String(obj.ptr());  // may be NULL
    auto py_bytes_obj = py::reinterpret_steal<py::object>(raw_bytes);
    return pythonBytesObjectToString(v8_isolate, py_bytes_obj.ptr());
  }

  if (PyBytes_CheckExact(obj.ptr())) {
    return pythonBytesObjectToString(v8_isolate, obj.ptr());
  }

  // please note that returning nullopt is different than
  // returning v8::Local<v8::String> with empty value inside
  // and that is different than returning v8::Local<v8::String> with empty string "" inside
  return std::nullopt;
}

v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const py::handle& py_str) {
  // first try to convert python string object directly if possible
  auto v8_str = toStringDirectly(v8_isolate, py_str);
  if (v8_str) {
    return *v8_str;
  }

  // alternatively convert it to string representation and try again
  auto raw_computed_str = PyObject_Str(py_str.ptr());  // may be NULL
  auto py_computed_obj = py::reinterpret_steal<py::object>(raw_computed_str);
  v8_str = toStringDirectly(v8_isolate, py_computed_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const std::wstring& str) {
  auto raw_unicode = PyUnicode_FromWideChar(str.c_str(), str.size());  // may be NULL
  auto py_obj = py::reinterpret_steal<py::object>(raw_unicode);
  auto v8_str = toStringDirectly(v8_isolate, py_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const std::string& str) {
  return v8::String::NewFromUtf8(v8_isolate, str.c_str(), v8::NewStringType::kNormal, str.size()).ToLocalChecked();
}

v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const char* s) {
  return v8::String::NewFromUtf8(v8_isolate, s, v8::NewStringType::kNormal).ToLocalChecked();
}

v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const std::string_view& sv) {
  return v8::String::NewFromUtf8(v8_isolate, sv.data(), v8::NewStringType::kNormal, sv.size()).ToLocalChecked();
}

v8::Local<v8::Integer> toPositiveInteger(LockedIsolatePtr& v8_isolate, int i) {
  if (i >= 0) {
    return v8::Integer::New(v8_isolate, i);
  } else {
    return v8::Local<v8::Integer>();  // empty value
  }
}

v8::String::Utf8Value toUTF(LockedIsolatePtr& v8_isolate, v8::Local<v8::Value> v8_value) {
  // the Utf8Value is not copyable, so we have to construct it twice for this check (dev mode only)
  assert(*v8::String::Utf8Value(v8_isolate, v8_value));
  return v8::String::Utf8Value(v8_isolate, v8_value);
}

std::string toStdString(LockedIsolatePtr& v8_isolate, v8::Local<v8::Value> v8_value) {
  if (v8_value.IsEmpty()) {
    return "";
  }
  auto v8_utf = toUTF(v8_isolate, v8_value);
  return std::string{*v8_utf, v8_utf.length()};
}

LockedIsolatePtr getCurrentIsolate() {
  // note in debug mode there is internal check in v8::Isolate::GetCurrent(), so this fails when there is no current
  // isolate
  return lockIsolate(v8::Isolate::GetCurrent());
}

v8::Isolate* getCurrentIsolateUnchecked() {
  return v8::Isolate::GetCurrent();
}

v8::Local<v8::Context> getCurrentContext(LockedIsolatePtr& v8_isolate) {
  assert(hasScope(v8_isolate));
  auto v8_context = v8_isolate->GetCurrentContext();
  if (v8_context.IsEmpty()) {
    throw JSException(v8_isolate, "Javascript object out of context", PyExc_UnboundLocalError);
  }
  return v8_context;
}

v8::Local<v8::Context> getCurrentContextUnchecked(LockedIsolatePtr& v8_isolate) {
  assert(hasScope(v8_isolate));
  return v8_isolate->GetCurrentContext();
}

ObservedHandleScope withScope(LockedIsolatePtr& v8_isolate) {
  return ObservedHandleScope(v8_isolate);
}

ObservedEscapableHandleScope withEscapableScope(LockedIsolatePtr& v8_isolate) {
  return ObservedEscapableHandleScope(v8_isolate);
}

bool hasScope(LockedIsolatePtr& v8_isolate) {
  return getCurrentHandleScopeLevel(v8_isolate) > 0;
}

v8::TryCatch withTryCatch(LockedIsolatePtr& v8_isolate) {
  return v8::TryCatch(v8_isolate);
}

void checkTryCatch(LockedIsolatePtr& v8_isolate, TryCatchPtr v8_try_catch) {
  JSException::CheckTryCatch(v8_isolate, v8_try_catch);
}

ProtectedIsolatePtr createIsolate() {
  v8::Isolate::CreateParams v8_create_params;
  v8_create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  auto v8_isolate = v8::Isolate::New(v8_create_params);
  assert(v8_isolate);
  return ProtectedIsolatePtr(v8_isolate);
}

v8::Context::Scope withContext(v8::Local<v8::Context> v8_context) {
  return v8::Context::Scope(v8_context);
}

v8::ScriptOrigin createScriptOrigin(v8::Local<v8::Value> v8_name,
                                    v8::Local<v8::Integer> v8_line,
                                    v8::Local<v8::Integer> v8_col) {
  return v8::ScriptOrigin(v8_name, v8_line, v8_col);
}

v8::Eternal<v8::Private> createEternalPrivateAPI(LockedIsolatePtr& v8_isolate, const char* name) {
  auto v8_key = v8::String::NewFromUtf8(v8_isolate, name).ToLocalChecked();
  auto v8_private_api = v8::Private::ForApi(v8_isolate, v8_key);
  return v8::Eternal<v8::Private>(v8_isolate, v8_private_api);
}

LockedIsolatePtr lockIsolate(v8::Isolate* v8_isolate) {
  return JSIsolate::FromV8(v8_isolate)->ToV8();
}

v8::Eternal<v8::String> createEternalString(LockedIsolatePtr& v8_isolate, const char* s) {
  return v8::Eternal<v8::String>(v8_isolate, toString(v8_isolate, s));
}

}  // namespace v8x