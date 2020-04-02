#include "Base.h"
#include "PythonUtils.h"

std::optional<v8::Local<v8::String>> toStringDirectly(py::object obj) {
  if (PyUnicode_CheckExact(obj.ptr())) {
    auto py_bytes = PyUnicode_AsUTF8String(obj.ptr());  // may be NULL
    py::object bytes_obj(py::handle<>(py::allow_null(py_bytes)));
    return pythonBytesObjectToString(bytes_obj.ptr());
  }

  if (PyBytes_CheckExact(obj.ptr())) {
    return pythonBytesObjectToString(obj.ptr());
  }

  // please note that returning nullopt is different than
  // returning v8::Local<v8::String> with empty value inside
  // and that is different than returning v8::Local<v8::String> with empty string "" inside
  return std::nullopt;
}

v8::Local<v8::String> ToString(py::object obj) {
  // first try to convert python string object directly if possible
  auto v8_str = toStringDirectly(obj);
  if (v8_str) {
    return *v8_str;
  }

  // alternatively convert it to string representation and try again
  auto py_str = PyObject_Str(obj.ptr());  // may be NULL
  auto str_obj = py::object(py::handle<>(py::allow_null(py_str)));
  v8_str = toStringDirectly(str_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::Local<v8::String> ToString(const std::string& str) {
  auto isolate = v8::Isolate::GetCurrent();
  return v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal, str.size()).ToLocalChecked();
}

v8::Local<v8::String> ToString(const std::wstring& str) {
  auto py_unicode = PyUnicode_FromWideChar(str.c_str(), str.size());  // may be NULL
  auto str_obj = py::object(py::handle<>(py::allow_null(py_unicode)));
  auto v8_str = toStringDirectly(str_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

CPythonGIL::CPythonGIL() {
  m_state = ::PyGILState_Ensure();
}

CPythonGIL::~CPythonGIL() {
  ::PyGILState_Release(m_state);
}
