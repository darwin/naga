#pragma once

#include "Base.h"

typedef v8::Persistent<v8::Object> V8Object;
typedef std::map<PyObject*, V8Object*> TrackingMap;

void traceV8Object(PyObject* raw_object, v8::Local<v8::Object> v8_object);
v8::Local<v8::Object> lookupTracedV8Object(PyObject* raw_object);

class CTracer {
  TrackingMap m_tracked_objects;

 public:
  void Dispose(PyObject* raw_object);
  void Trace(PyObject* raw_object, v8::Local<v8::Object> v8_object);
  v8::Local<v8::Object> Lookup(PyObject* raw_object);

  static void WeakCallback(const v8::WeakCallbackInfo<PyObject>& v8_info);
};