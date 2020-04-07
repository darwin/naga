#pragma once

#include "Base.h"

class CPythonObject {
 public:
  static void NamedGetter(v8::Local<v8::Name> v8_prop_name, const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void NamedSetter(v8::Local<v8::Name> v8_prop_name,
                          v8::Local<v8::Value> v8_value,
                          const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void NamedQuery(v8::Local<v8::Name> v8_prop_name, const v8::PropertyCallbackInfo<v8::Integer>& v8_info);
  static void NamedDeleter(v8::Local<v8::Name> v8_prop_name, const v8::PropertyCallbackInfo<v8::Boolean>& v8_info);
  static void NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info);

  static void IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void IndexedSetter(uint32_t index,
                            v8::Local<v8::Value> v8_value,
                            const v8::PropertyCallbackInfo<v8::Value>& v8_info);
  static void IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& v8_info);
  static void IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& v8_info);
  static void IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info);

  static void Caller(const v8::FunctionCallbackInfo<v8::Value>& v8_info);

  static void SetupObjectTemplate(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> clazz);
  static v8::Local<v8::ObjectTemplate> CreateObjectTemplate(v8::Isolate* isolate);
  static v8::Local<v8::ObjectTemplate> GetCachedObjectTemplateOrCreate(v8::Isolate* isolate);

  static v8::Local<v8::Value> WrapInternal2(py::handle py_obj);

  static bool IsWrapped2(v8::Local<v8::Object> v8_obj);
  static v8::Local<v8::Value> Wrap(py::handle py_obj);
  static py::object GetWrapper2(v8::Local<v8::Object> v8_obj);

  static void Dispose(v8::Local<v8::Value> v8_val);

  static void ThrowIf(v8::Isolate* v8_isolate, const py::error_already_set& e = py::error_already_set());
};