#ifndef NAGA_PYTHONOBJECT_H_
#define NAGA_PYTHONOBJECT_H_

#include "Base.h"

class CPythonObject {
 public:
  static void NamedGetter(v8::Local<v8::Name> v8_name, const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void NamedSetter(v8::Local<v8::Name> v8_name,
                          v8::Local<v8::Value> v8_value,
                          const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void NamedQuery(v8::Local<v8::Name> v8_name, const v8::PropertyCallbackInfo<v8::Integer>& v8_info);
  static void NamedDeleter(v8::Local<v8::Name> v8_name, const v8::PropertyCallbackInfo<v8::Boolean>& v8_info);
  static void NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info);

  static void IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void IndexedSetter(uint32_t index,
                            v8::Local<v8::Value> v8_value,
                            const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& v8_info);
  static void IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& v8_info);
  static void IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info);

  static void CallWrapperAsFunction(const v8::FunctionCallbackInfo<v8::Value>& v8_info);
  static void CallPythonCallable(const py::object& py_fn, const v8::FunctionCallbackInfo<v8::Value>& v8_info);

  static v8::Local<v8::ObjectTemplate> CreateJSWrapperTemplate(v8::IsolatePtr v8_isolate);
  static v8::Local<v8::ObjectTemplate> GetOrCreateCachedJSWrapperTemplate(v8::IsolatePtr v8_isolate);

  static void ThrowJSException(v8::IsolatePtr v8_isolate, const py::error_already_set& py_ex = py::error_already_set());
};

#endif