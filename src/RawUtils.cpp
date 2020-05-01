#include "RawUtils.h"
#include "V8Utils.h"

v8::Local<v8::String> pythonBytesObjectToString(PyObject* raw_obj) {
  if (!raw_obj) {
    return v8::Local<v8::String>();
  }

  assert(PyBytes_CheckExact(raw_obj));
  auto s = PyBytes_AS_STRING(raw_obj);
  assert(s);
  auto sz = PyBytes_GET_SIZE(raw_obj);
  assert(sz >= 0);
  auto v8_isolate = v8u::getCurrentIsolate();
  return v8::String::NewFromUtf8(v8_isolate, s, v8::NewStringType::kNormal, sz).ToLocalChecked();
}

const char* pythonTypeName(PyTypeObject* raw_type) {
  return raw_type->tp_name;
}
