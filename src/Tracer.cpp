#include "Tracer.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kV8TracingLogger), __VA_ARGS__)

// TODO: do proper cleanup when isolate goes away
// currently we rely on weak callbacks per tracked object
// these callbacks are not guaranteed by V8, we should do extra cleanup step during isolate destruction
// see https://itnext.io/v8-wrapped-objects-lifecycle-42272de712e0
//
// also to limit number of weak callbacks we could set weak callback only per-context and dispose all
// context-related objects during context disposal (this is how original STPyV8 tracking worked)

static CTracer g_tracer;

void traceV8Object(PyObject* raw_object, v8::Local<v8::Object> v8_object) {
  g_tracer.Trace(raw_object, v8_object);
}

v8::Local<v8::Object> lookupTracedV8Object(PyObject* raw_object) {
  return g_tracer.Lookup(raw_object);
}

// --------------------------------------------------------------------------------------------------------------------

void CTracer::Trace(PyObject* raw_object, v8::Local<v8::Object> v8_object) {
  TRACE("CTracer::Trace raw_object={} v8_object={}", raw_object_printer{raw_object}, v8_object);

  // trace must be called only for not-yet tracked objects
  assert(m_tracked_objects.find(raw_object) == m_tracked_objects.end());
  auto v8_isolate = v8u::getCurrentIsolate();

  // create persistent V8 object and mark it as weak
  auto v8_object_ptr = new V8Object(v8_isolate, v8_object);
  v8_object_ptr->SetWeak(raw_object, WeakCallback, v8::WeakCallbackType::kParameter);

  // hold onto raw PyObject
  Py_INCREF(raw_object);

  // add our record for lookups
  m_tracked_objects.insert(std::make_pair(raw_object, v8_object_ptr));
}

v8::Local<v8::Object> CTracer::Lookup(PyObject* raw_object) {
  auto lookup = m_tracked_objects.find(raw_object);
  if (lookup == m_tracked_objects.end()) {
    TRACE("CTracer::Lookup raw_object={} => CACHE MISS", raw_object_printer{raw_object});
    return v8::Local<v8::Object>();
  }
  auto v8_result = v8::Local<v8::Object>::New(v8u::getCurrentIsolate(), *lookup->second);
  TRACE("CTracer::Lookup raw_object={} => {}", raw_object_printer{raw_object}, v8_result);
  return v8_result;
}

void CTracer::Dispose(PyObject* raw_object) {
  TRACE("CTracer::Dispose raw_object={}", raw_object_printer{raw_object});

  // dispose mus tbe called for a tracked object
  auto lookup = m_tracked_objects.find(raw_object);
  assert(lookup != m_tracked_objects.end());

  // dispose V8 object
  auto v8_object_ptr = lookup->second;
  v8_object_ptr->Reset();
  delete v8_object_ptr;

  // dispose PyObject
  Py_DECREF(raw_object);

  // remove our record
  m_tracked_objects.erase(lookup);
}

void CTracer::WeakCallback(const v8::WeakCallbackInfo<PyObject>& v8_info) {
  auto raw_object = v8_info.GetParameter();
  TRACE("CTracer::WeakCallback raw_object={}", raw_object_printer{raw_object});
  g_tracer.Dispose(raw_object);
}
