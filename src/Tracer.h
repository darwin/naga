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

// ---------------------------------------------------------------------

class ObjectTracer2;

typedef std::map<PyObject*, ObjectTracer2*> LivingMap2;

class ObjectTracer2 {
  v8::Persistent<v8::Value> m_handle;
  std::unique_ptr<pb::object> m_object;

  LivingMap2* m_living;

  void Trace();

  static LivingMap2* GetLivingMapping();

 public:
  ObjectTracer2(v8::Local<v8::Value> handle, pb::object* object);
  ~ObjectTracer2();

  pb::object* Object() const { return m_object.get(); }

  void Dispose();

  static ObjectTracer2& Trace(v8::Local<v8::Value> handle, pb::object* object);

  static v8::Local<v8::Value> FindCache(pb::handle obj);
};

class ContextTracer2 {
  v8::Persistent<v8::Context> m_ctxt;
  std::unique_ptr<LivingMap2> m_living;

  void Trace();

  static void WeakCallback(const v8::WeakCallbackInfo<ContextTracer2>& info);

 public:
  ContextTracer2(v8::Local<v8::Context> ctxt, LivingMap2* living);
  ~ContextTracer2();

  v8::Local<v8::Context> Context() const { return v8::Local<v8::Context>::New(v8::Isolate::GetCurrent(), m_ctxt); }

  static void Trace(v8::Local<v8::Context> ctxt, LivingMap2* living);
};