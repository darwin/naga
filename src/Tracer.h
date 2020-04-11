#pragma once

#include "Base.h"

class ObjectTracer;

typedef std::map<PyObject*, ObjectTracer*> LivingMap;

class ObjectTracer {
  v8::Persistent<v8::Value> m_v8_handle;
  PyObject* m_raw_object;
  LivingMap* m_living_ptr;

  void Trace();

  static LivingMap* GetLivingMap();

 public:
  ObjectTracer(v8::Local<v8::Value> v8_handle, PyObject* raw_object);
  ~ObjectTracer();

  [[nodiscard]] py::object Object() const;

  void Dispose();
  static ObjectTracer& Trace(v8::Local<v8::Value> v8_handle, PyObject* raw_object);
  static v8::Local<v8::Value> FindCache(PyObject* raw_object);
};

class ContextTracer {
  v8::Persistent<v8::Context> m_v8_context;
  std::unique_ptr<LivingMap> m_living;

  void Trace();
  static void WeakCallback(const v8::WeakCallbackInfo<ContextTracer>& v8_info);

 public:
  ContextTracer(v8::Local<v8::Context> v8_context, LivingMap* living);
  ~ContextTracer();

  [[nodiscard]] v8::Local<v8::Context> Context() const;

  static void Trace(v8::Local<v8::Context> v8_context, LivingMap* living);
};