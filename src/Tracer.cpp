#include "Tracer.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kV8TracingLogger), __VA_ARGS__)

#define PyType_SUPPORTS_WEAKREFS(t) ((t)->tp_weaklistoffset > 0)

// TODO: do proper cleanup when isolate goes away
// currently we rely on weak callbacks per tracked object
// these callbacks are not guaranteed by V8, we should do extra cleanup step during isolate destruction
// see https://itnext.io/v8-wrapped-objects-lifecycle-42272de712e0

static CTracer g_tracer;

void traceWrapper(TracedRawObject* raw_object, v8::Local<v8::Object> v8_wrapper) {
  TRACE("traceWrapper raw_object={} v8_wrapper={}", py::handle(raw_object), v8_wrapper);
  g_tracer.TraceWrapper(raw_object, v8_wrapper);
}

v8::Local<v8::Object> lookupTracedWrapper(TracedRawObject* raw_object) {
  auto v8_result = g_tracer.LookupWrapper(raw_object);
  TRACE("lookupTracedWrapper raw_object={}", py::handle(raw_object), v8_result);
  return v8_result;
}

TracedRawObject* detectTracedWrapper(v8::Local<v8::Object> v8_wrapper) {
  TRACE("detectTracedWrapper v8_wrapper={}", v8_wrapper);
  if (v8_wrapper.IsEmpty()) {
    return nullptr;
  }
  auto v8_isolate = v8u::getCurrentIsolate();
  // TODO: optimize
  auto v8_payload_key = v8::String::NewFromUtf8(v8_isolate, "tracer_payload").ToLocalChecked();
  auto v8_payload_api = v8::Private::ForApi(v8_isolate, v8_payload_key);
  if (!v8_wrapper->HasPrivate(v8_isolate->GetCurrentContext(), v8_payload_api).ToChecked()) {
    return nullptr;
  }
  auto v8_val = v8_wrapper->GetPrivate(v8_isolate->GetCurrentContext(), v8_payload_api).ToLocalChecked();
  assert(!v8_val.IsEmpty());
  assert(v8_val->IsExternal());
  auto v8_payload = v8_val.As<v8::External>();
  auto raw_obj = static_cast<PyObject*>(v8_payload->Value());
  TRACE("detectTracedWrapper => {}", py::handle(raw_obj));
  return raw_obj;
}

static void recordTracedWrapper(v8::Local<v8::Object> v8_wrapper, TracedRawObject* raw_object) {
  TRACE("recordTracedWrapper v8_wrapper={} raw_object={}", v8_wrapper, raw_object_printer{raw_object});
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_context = v8_isolate->GetCurrentContext();
  // TODO: optimize
  auto v8_payload_key = v8::String::NewFromUtf8(v8_isolate, "tracer_payload").ToLocalChecked();
  auto v8_payload_api = v8::Private::ForApi(v8_isolate, v8_payload_key);
  auto v8_payload = v8::External::New(v8_isolate, raw_object);
  v8_wrapper->SetPrivate(v8_context, v8_payload_api, v8_payload);
}

static void pythonWeakRefCallback(const py::handle& py_weak_ref) {
  TRACE("pythonWeakRefCallback py_weak_ref={}", py_weak_ref);
  g_tracer.KillZombie(py_weak_ref.ptr());
}

static void v8WeakCallback(const v8::WeakCallbackInfo<TracedRawObject>& data) {
  // note that this call may come from anywhere, so we have to grab the GIL for potential Python API call
  auto py_gil = pyu::withGIL();

  auto raw_object = data.GetParameter();
  TRACE("v8WeakCallback data.GetParameter={}", raw_object_printer{raw_object});
  g_tracer.AssociatedWrapperObjectIsAboutToDie(raw_object);
}

// --------------------------------------------------------------------------------------------------------------------

void CTracer::TraceWrapper(TracedRawObject* raw_object, v8::Local<v8::Object> v8_wrapper) {
  TRACE("CTracer::TraceWrapper raw_object={} v8_object={}", raw_object_printer{raw_object}, v8_wrapper);

  assert(raw_object);

  // trace must be called only for not-yet tracked objects
  assert(m_tracked_wrappers.find(raw_object) == m_tracked_wrappers.end());

  // create persistent V8 object and mark it as weak
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_object_ptr = new V8Wrapper(v8_isolate, v8_wrapper);

  // add our record for lookups
  recordTracedWrapper(v8_wrapper, raw_object);
  auto insert_point = m_tracked_wrappers.insert(std::make_pair(raw_object, TracerRecord{v8_object_ptr, nullptr}));

  // start in live mode and we know we don't have to do any cleanup
  SwitchToLiveMode(insert_point.first, false);
}

v8::Local<v8::Object> CTracer::LookupWrapper(PyObject* raw_object) {
  auto tracer_lookup = m_tracked_wrappers.find(raw_object);
  if (tracer_lookup == m_tracked_wrappers.end()) {
    TRACE("CTracer::LookupWrapper raw_object={} => CACHE MISS", raw_object_printer{raw_object});
    return v8::Local<v8::Object>();
  }

  // we we are passing zombie wrapper, we have to make it live again
  if (tracer_lookup->second.m_weak_ref) {
    SwitchToLiveMode(tracer_lookup);
  }

  auto v8_result = v8::Local<v8::Object>::New(v8u::getCurrentIsolate(), *tracer_lookup->second.m_v8_wrapper);
  TRACE("CTracer::LookupWrapper => {}", v8_result);
  return v8_result;
}

void CTracer::KillZombie(WeakRefRawObject* raw_weak_ref) {
  TRACE("CTracer::KillZombie raw_weak_ref={}", raw_object_printer{raw_weak_ref});

  auto lookup_weak_ref = m_weak_refs.find(raw_weak_ref);
  assert(lookup_weak_ref != m_weak_refs.end());

  KillWrapper(lookup_weak_ref->second);

  // remove our records
  m_weak_refs.erase(lookup_weak_ref);
  Py_DECREF(raw_weak_ref);
}

void CTracer::KillWrapper(PyObject* raw_object) {
  TRACE("CTracer::KillWrapper raw_object={}", raw_object_printer{raw_object});

  auto tracer_lookup = m_tracked_wrappers.find(raw_object);
  assert(tracer_lookup != m_tracked_wrappers.end());

  // dispose the V8 object
  auto v8_object_ptr = tracer_lookup->second.m_v8_wrapper;
  v8_object_ptr->Reset();
  delete v8_object_ptr;

  // remove our record
  m_tracked_wrappers.erase(tracer_lookup);
}

void CTracer::SwitchToLiveMode(WrapperTrackingMap::iterator tracer_lookup, bool cleanup) {
  TRACE("CTracer::SwitchToLiveMode");

  auto raw_object = tracer_lookup->first;

  // mark V8 wrapper as weak
  auto v8_object_ptr = tracer_lookup->second.m_v8_wrapper;
  assert(!tracer_lookup->second.m_v8_wrapper->IsWeak());
  v8_object_ptr->SetWeak(raw_object, &v8WeakCallback, v8::WeakCallbackType::kFinalizer);

  // hold onto the Python object strongly
  Py_INCREF(raw_object);

  // perform cleanup of old weak ref, if requested
  if (cleanup) {
    auto raw_weak_ref = tracer_lookup->second.m_weak_ref;
    assert(raw_weak_ref);
    m_weak_refs.erase(raw_weak_ref);
    Py_DECREF(raw_weak_ref);
    tracer_lookup->second.m_weak_ref = nullptr;
  } else {
    assert(!tracer_lookup->second.m_weak_ref);
  }
}

void CTracer::SwitchToZombieModeOrDie(TracedRawObject* raw_object) {
  TRACE("CTracer::SwitchToZombieModeOrDie raw_object={}", raw_object_printer{raw_object});

  // there must be someone else holding this object as well
  assert(Py_REFCNT(raw_object) > 1);

  // note that not all Python objects support weak references
  // in this case we cannot keep V8 wrapper alive
  // subsequent possible lookups for V8 wrapper will have to recreate the wrapper again
  if (!PyType_SUPPORTS_WEAKREFS(Py_TYPE(raw_object))) {
    // TODO: support better printing for Python types
    TRACE("CTracer::SwitchToZombieMode weak reference is not possible for {}",
          py::handle((PyObject*)Py_TYPE(raw_object)));
    KillWrapper(raw_object);
    Py_DECREF(raw_object);
    return;
  }

  auto tracer_lookup = m_tracked_wrappers.find(raw_object);
  assert(tracer_lookup != m_tracked_wrappers.end());

  SwitchToZombieMode(tracer_lookup);
}

void CTracer::SwitchToZombieMode(WrapperTrackingMap::iterator tracer_lookup) {
  auto raw_object = tracer_lookup->first;

  // this will resurrect the V8 object and keep it alive until we kill it
  tracer_lookup->second.m_v8_wrapper->ClearWeak();
  tracer_lookup->second.m_v8_wrapper->AnnotateStrongRetainer("STPyV8.Tracer");

  // ask for a weakref to the Python object
  static py::cpp_function g_py_weak_ref_callback(&pythonWeakRefCallback);
  auto raw_weak_ref = PyWeakref_NewRef(raw_object, g_py_weak_ref_callback.ptr());
  assert(raw_weak_ref);

  // update our records
  assert(!tracer_lookup->second.m_weak_ref);
  tracer_lookup->second.m_weak_ref = raw_weak_ref;
  m_weak_refs.insert(std::make_pair(raw_weak_ref, raw_object));

  // stop holding the Python object strongly
  // we will be called back via a callback when the Python object is about to die
  Py_DECREF(raw_object);
}

void CTracer::AssociatedWrapperObjectIsAboutToDie(TracedRawObject* raw_object) {
  TRACE("CTracer::AssociatedWrapperObjectIsAboutToDie raw_object={}", raw_object_printer{raw_object});

  if (Py_REFCNT(raw_object) == 1) {
    // this is a fast path
    // if we are the last one holding the Python object
    // we can go ahead and let the V8 object die and release the Python object directly
    KillWrapper(raw_object);
    Py_DECREF(raw_object);
  } else {
    // if there is someone else still holding the Python object
    // we want to keep our V8 object alive until the Python object goes away
    // we switch our Python object to weak mode and observe its release
    SwitchToZombieModeOrDie(raw_object);
  }
}