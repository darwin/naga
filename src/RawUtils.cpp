#include "_precompile.h"

#include "RawUtils.h"

v8::Local<v8::String> pythonBytesObjectToString(PyObject* py_obj) {
  if (!py_obj) {
    return v8::Local<v8::String>();
  }

  assert(PyBytes_CheckExact(py_obj));
  auto s = PyBytes_AS_STRING(py_obj);
  assert(s);
  auto sz = PyBytes_GET_SIZE(py_obj);
  assert(sz >= 0);
  auto v8_isolate = v8u::getCurrentIsolate();
  return v8::String::NewFromUtf8(v8_isolate, s, v8::NewStringType::kNormal, sz).ToLocalChecked();
}