#include "V8Utils.h"
#include "PythonUtils.h"
#include "JSException.h"

namespace v8u {

//std::optional<v8::Local<v8::String>> toStringDirectly(py::object obj) {
//  if (PyUnicode_CheckExact(obj.ptr())) {
//    auto py_bytes = PyUnicode_AsUTF8String(obj.ptr());  // may be NULL
//    py::object bytes_obj(py::handle<>(py::allow_null(py_bytes)));
//    return pythonBytesObjectToString(bytes_obj.ptr());
//  }
//
//  if (PyBytes_CheckExact(obj.ptr())) {
//    return pythonBytesObjectToString(obj.ptr());
//  }
//
//  // please note that returning nullopt is different than
//  // returning v8::Local<v8::String> with empty value inside
//  // and that is different than returning v8::Local<v8::String> with empty string "" inside
//  return std::nullopt;
//}

std::optional<v8::Local<v8::String>> toStringDirectly(pb::handle obj) {
  if (PyUnicode_CheckExact(obj.ptr())) {
    auto py_bytes = PyUnicode_AsUTF8String(obj.ptr());  // may be NULL
    auto bytes_obj = pb::reinterpret_steal<pb::object>(py_bytes);
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

//v8::Local<v8::String> toString(py::object str) {
//  // first try to convert python string object directly if possible
//  auto v8_str = toStringDirectly(str);
//  if (v8_str) {
//    return *v8_str;
//  }
//
//  // alternatively convert it to string representation and try again
//  auto py_str = PyObject_Str(str.ptr());  // may be NULL
//  auto str_obj = py::object(py::handle<>(py::allow_null(py_str)));
//  v8_str = toStringDirectly(str_obj);
//  if (v8_str) {
//    return *v8_str;
//  }
//
//  // all attempts failed, return empty value
//  return v8::Local<v8::String>();
//}

v8::Local<v8::String> toString(pb::handle str) {
  // first try to convert python string object directly if possible
  auto v8_str = toStringDirectly(str);
  if (v8_str) {
    return *v8_str;
  }

  // alternatively convert it to string representation and try again
  auto py_str = PyObject_Str(str.ptr());  // may be NULL
  auto str_obj = pb::reinterpret_steal<pb::object>(py_str);
  v8_str = toStringDirectly(str_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::Local<v8::String> toString(const std::string& str) {
  auto isolate = v8::Isolate::GetCurrent();
  return v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal, str.size()).ToLocalChecked();
}

v8::Local<v8::String> toString(const std::wstring& str) {
  auto raw_unicode = PyUnicode_FromWideChar(str.c_str(), str.size());  // may be NULL
  auto py_obj = pb::reinterpret_steal<pb::object>(raw_unicode);
  auto v8_str = toStringDirectly(py_obj);
  if (v8_str) {
    return *v8_str;
  }

  // all attempts failed, return empty value
  return v8::Local<v8::String>();
}

v8::String::Utf8Value toUtf8Value(v8::Isolate* v8_isolate, v8::Local<v8::String> v8_string) {
  return v8::String::Utf8Value(v8_isolate, v8_string);
}

void checkContext(v8::Isolate* v8_isolate) {
  auto scope = getScope(v8_isolate);
  if (v8_isolate->GetCurrentContext().IsEmpty()) {
    throw CJSException(v8_isolate, "Javascript object out of context", PyExc_UnboundLocalError);
  }
}

bool executionTerminating(v8::Isolate* v8_isolate) {
  if (!v8_isolate->IsExecutionTerminating()) {
    return false;
  }

  PyErr_Clear();
  PyErr_SetString(PyExc_RuntimeError, "execution is terminating");
  return true;
}

v8::HandleScope getScope(v8::Isolate* v8_isolate) {
  return v8::HandleScope(v8_isolate);
}

v8::EscapableHandleScope openEscapableScope(v8::Isolate* v8_isolate) {
  return v8::EscapableHandleScope(v8_isolate);
}

v8::TryCatch openTryCatch(v8::Isolate* v8_isolate) {
  return v8::TryCatch(v8_isolate);
}

}  // namespace v8u

std::ostream& operator<<(std::ostream& os, v8::Local<v8::Value> v8_val) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_context = v8_isolate->GetCurrentContext();
  if (v8_val.IsEmpty()) {
    os << "[EMPTY VAL]";
  } else {
    auto v8_str = v8_val->ToDetailString(v8_context);
    if (!v8_str.IsEmpty()) {
      auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_str.ToLocalChecked());
      os << *v8_utf;
    } else {
      os << "[N/A]";
    }
  }
  return os;
}
