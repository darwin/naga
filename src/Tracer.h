#pragma once

#include "Base.h"

class ObjectTracer;

typedef std::map<PyObject*, ObjectTracer*> LivingMap2;

class ObjectTracer {
  v8::Persistent<v8::Value> m_v8_handle;
  PyObject* m_raw_object;
  LivingMap2* m_living;

  void Trace();

  static LivingMap2* GetLivingMapping();

 public:
  ObjectTracer(v8::Local<v8::Value> v8_handle, PyObject* raw_object);
  ~ObjectTracer();

  py::object Object() const;

  void Dispose();
  static ObjectTracer& Trace(v8::Local<v8::Value> v8_handle, PyObject* raw_object);
  static v8::Local<v8::Value> FindCache(PyObject* raw_object);
};

class ContextTracer {
  v8::Persistent<v8::Context> m_v8_context;
  std::unique_ptr<LivingMap2> m_living;

  void Trace();
  static void WeakCallback(const v8::WeakCallbackInfo<ContextTracer>& v8_info);

 public:
  ContextTracer(v8::Local<v8::Context> v8_context, LivingMap2* living);
  ~ContextTracer();

  v8::Local<v8::Context> Context() const;

  static void Trace(v8::Local<v8::Context> v8_context, LivingMap2* living);
};