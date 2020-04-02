#include "PythonUtils.h"

v8::Local<v8::String> pythonBytesObjectToString(PyObject* py_obj) {
  if (!py_obj) {
    return v8::Local<v8::String>();
  }

  assert(PyBytes_CheckExact(py_obj));
  auto s = PyBytes_AS_STRING(py_obj);
  assert(s);
  auto sz = PyBytes_GET_SIZE(py_obj);
  assert(sz >= 0);
  auto isolate = v8::Isolate::GetCurrent();
  return v8::String::NewFromUtf8(isolate, s, v8::NewStringType::kNormal, sz).ToLocalChecked();
}