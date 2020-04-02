#pragma once

#include "Base.h"

class ObjectTracer;

typedef std::map<PyObject*, ObjectTracer*> LivingMap;

class ObjectTracer {
  v8::Persistent<v8::Value> m_handle;
  std::unique_ptr<py::object> m_object;

  LivingMap* m_living;

  void Trace();

  static LivingMap* GetLivingMapping();

 public:
  ObjectTracer(v8::Local<v8::Value> handle, py::object* object);
  ~ObjectTracer();

  py::object* Object() const { return m_object.get(); }

  void Dispose();

  static ObjectTracer& Trace(v8::Local<v8::Value> handle, py::object* object);

  static v8::Local<v8::Value> FindCache(py::object obj);
};

class ContextTracer {
  v8::Persistent<v8::Context> m_ctxt;
  std::unique_ptr<LivingMap> m_living;

  void Trace();

  static void WeakCallback(const v8::WeakCallbackInfo<ContextTracer>& info);

 public:
  ContextTracer(v8::Local<v8::Context> ctxt, LivingMap* living);
  ~ContextTracer();

  v8::Local<v8::Context> Context() const { return v8::Local<v8::Context>::New(v8::Isolate::GetCurrent(), m_ctxt); }

  static void Trace(v8::Local<v8::Context> ctxt, LivingMap* living);
};