#pragma once

#include "Base.h"

class CPythonObject {
 private:
  CPythonObject();
  virtual ~CPythonObject();

 public:
  static void NamedGetter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Value>& info);
  static void NamedSetter(v8::Local<v8::Name> prop,
                          v8::Local<v8::Value> value,
                          const v8::PropertyCallbackInfo<v8::Value>& info);
  static void NamedQuery(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Integer>& info);
  static void NamedDeleter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Boolean>& info);
  static void NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info);

  static void IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info);
  static void IndexedSetter(uint32_t index,
                            v8::Local<v8::Value> value,
                            const v8::PropertyCallbackInfo<v8::Value>& info);
  static void IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& info);
  static void IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info);
  static void IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info);

  static void Caller(const v8::FunctionCallbackInfo<v8::Value>& info);

  static void SetupObjectTemplate(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> clazz);
  static v8::Local<v8::ObjectTemplate> CreateObjectTemplate(v8::Isolate* isolate);
  static v8::Local<v8::ObjectTemplate> GetCachedObjectTemplateOrCreate(v8::Isolate* isolate);

  static v8::Local<v8::Value> WrapInternal(py::object obj);

  static bool IsWrapped(v8::Local<v8::Object> obj);
  static v8::Local<v8::Value> Wrap(py::object obj);
  static py::object Unwrap(v8::Local<v8::Object> obj);
  static void Dispose(v8::Local<v8::Value> value);

  static void ThrowIf(v8::Isolate* isolate);
};