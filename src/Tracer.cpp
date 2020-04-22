#include "Tracer.h"
#include "Isolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kV8TracingLogger), __VA_ARGS__)

#define PyType_SUPPORTS_WEAKREFS(t) ((t)->tp_weaklistoffset > 0)

void traceWrapper(TracedRawObject* raw_object, v8::Local<v8::Object> v8_wrapper) {
  auto v8_isolate = v8_wrapper->GetIsolate();
  TRACE("traceWrapper v8_isolate={} raw_object={} v8_wrapper={}", P$(v8_isolate), py::handle(raw_object), v8_wrapper);
  auto isolate = CIsolate::FromV8(v8_isolate);
  isolate->Tracer()->TraceWrapper(raw_object, v8_wrapper);
}

v8::Local<v8::Object> lookupTracedWrapper(v8::IsolateRef v8_isolate, TracedRawObject* raw_object) {
  auto isolate = CIsolate::FromV8(v8_isolate);
  auto v8_result = isolate->Tracer()->LookupWrapper(v8_isolate, raw_object);
  TRACE("lookupTracedWrapper v8_isolate={} raw_object={}", P$(v8_isolate), py::handle(raw_object), v8_result);
  return v8_result;
}

TracedRawObject* lookupTracedObject(v8::Local<v8::Object> v8_wrapper) {
  TRACE("lookupTracedObject v8_wrapper={}", v8_wrapper);

  if (v8_wrapper.IsEmpty()) {
    return nullptr;
  }
  auto v8_isolate = v8_wrapper->GetIsolate();

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
  TRACE("lookupTracedObject => {}", raw_obj);
  return raw_obj;
}

static void recordTracedWrapper(v8::Local<v8::Object> v8_wrapper, TracedRawObject* raw_object) {
  TRACE("recordTracedWrapper v8_wrapper={} raw_object={}", v8_wrapper, raw_object);
  auto v8_isolate = v8_wrapper->GetIsolate();
  auto v8_context = v8_isolate->GetCurrentContext();
  // TODO: optimize
  auto v8_payload_key = v8::String::NewFromUtf8(v8_isolate, "tracer_payload").ToLocalChecked();
  auto v8_payload_api = v8::Private::ForApi(v8_isolate, v8_payload_key);
  auto v8_payload = v8::External::New(v8_isolate, raw_object);
  v8_wrapper->SetPrivate(v8_context, v8_payload_api, v8_payload);
}

static void v8WeakCallback(const v8::WeakCallbackInfo<TracedRawObject>& data) {
  // note that this call may come from anywhere, so we have to grab the GIL for potential Python API calls
  auto py_gil = pyu::withGIL();
  auto v8_isolate = data.GetIsolate();
  auto isolate = CIsolate::FromV8(v8_isolate);
  auto raw_object = data.GetParameter();
  TRACE("v8WeakCallback data.GetParameter={} v8_isolate={}", raw_object, P$(v8_isolate));
  isolate->Tracer()->AssociatedWrapperObjectIsAboutToDie(raw_object);
}

// --------------------------------------------------------------------------------------------------------------------

CTracer::CTracer() {
  TRACE("CTracer::CTracer {}", THIS);
}

CTracer::~CTracer() {
  TRACE("CTracer::~CTracer {}", THIS);
  // make sure we release all Python objects in live mode and drop all pending weakrefs
  auto it = m_tracked_wrappers.begin();
  while (it != m_tracked_wrappers.end()) {
    if (!it->second.m_weak_ref) {
      // live mode
      Py_DECREF(it->first);
    } else {
      // zombie mode
      Py_DECREF(it->second.m_weak_ref);
    }
    it++;
  }
}

void CTracer::TraceWrapper(TracedRawObject* raw_object, v8::Local<v8::Object> v8_wrapper) {
  TRACE("CTracer::TraceWrapper {} raw_object={} v8_object={}", THIS, raw_object, v8_wrapper);

  assert(raw_object);

  // trace must be called only for not-yet tracked objects
  assert(m_tracked_wrappers.find(raw_object) == m_tracked_wrappers.end());

  auto v8_isolate = v8_wrapper->GetIsolate();

  // add our record for lookups
  recordTracedWrapper(v8_wrapper, raw_object);
  auto insert_point =
      m_tracked_wrappers.insert(std::make_pair(raw_object, TracerRecord{V8Wrapper(v8_isolate, v8_wrapper), nullptr}));

  // start in live mode and we know we don't have to do any cleanup
  SwitchToLiveMode(insert_point.first, false);
}

v8::Local<v8::Object> CTracer::LookupWrapper(v8::IsolateRef v8_isolate, PyObject* raw_object) {
  auto tracer_lookup = m_tracked_wrappers.find(raw_object);
  if (tracer_lookup == m_tracked_wrappers.end()) {
    TRACE("CTracer::LookupWrapper {} raw_object={} => CACHE MISS", THIS, raw_object);
    return v8::Local<v8::Object>();
  }

  auto v8_result = tracer_lookup->second.m_v8_wrapper.Get(v8_isolate);

  // we we are passing zombie wrapper, we have to make it live again
  // don't do this before the zombie is referenced by v8_result
  if (tracer_lookup->second.m_weak_ref) {
    SwitchToLiveMode(tracer_lookup);
  }

  TRACE("CTracer::LookupWrapper {} => {}", THIS, v8_result);
  return v8_result;
}

void CTracer::DeleteRecord(PyObject* raw_object) {
  TRACE("CTracer::DeleteRecord {} raw_object={}", THIS, raw_object);

  auto tracer_lookup = m_tracked_wrappers.find(raw_object);
  assert(tracer_lookup != m_tracked_wrappers.end());

  auto& raw_weak_ref = tracer_lookup->second.m_weak_ref;
  if (raw_weak_ref) {
    Py_DECREF(raw_weak_ref);
    raw_weak_ref = nullptr;
  }

  // remove our record
  // note that m_v8_wrapper.Reset() will be called in its destructor
  m_tracked_wrappers.erase(tracer_lookup);
}

void CTracer::SwitchToLiveMode(WrapperTrackingMap::iterator tracer_lookup, bool cleanup) {
  TRACE("CTracer::SwitchToLiveMode {}", THIS);

  auto raw_object = tracer_lookup->first;

  // mark V8 wrapper as weak
  assert(!tracer_lookup->second.m_v8_wrapper.IsWeak());
  tracer_lookup->second.m_v8_wrapper.SetWeak(raw_object, &v8WeakCallback, v8::WeakCallbackType::kFinalizer);

  // hold onto the Python object strongly
  Py_INCREF(raw_object);

  // perform cleanup of old weak ref, if requested
  if (cleanup) {
    auto& raw_weak_ref = tracer_lookup->second.m_weak_ref;
    assert(raw_weak_ref);
    Py_DECREF(raw_weak_ref);
    raw_weak_ref = nullptr;
  } else {
    assert(!tracer_lookup->second.m_weak_ref);
  }
}

void CTracer::SwitchToZombieModeOrDie(TracedRawObject* raw_object) {
  TRACE("CTracer::SwitchToZombieModeOrDie {} raw_object={}", THIS, raw_object);

  // there must be someone else holding this object as well
  assert(Py_REFCNT(raw_object) > 1);

  // note that not all Python objects support weak references
  // in this case we cannot keep V8 wrapper alive
  // subsequent possible lookups for V8 wrapper will have to recreate the wrapper again
  if (!PyType_SUPPORTS_WEAKREFS(Py_TYPE(raw_object))) {
    // TODO: support better printing for Python types
    TRACE("CTracer::SwitchToZombieMode weak reference is not possible for {}",
          py::handle((PyObject*)Py_TYPE(raw_object)));
    DeleteRecord(raw_object);
    Py_DECREF(raw_object);
    return;
  }

  auto tracer_lookup = m_tracked_wrappers.find(raw_object);
  assert(tracer_lookup != m_tracked_wrappers.end());

  SwitchToZombieMode(tracer_lookup);
}

void CTracer::SwitchToZombieMode(WrapperTrackingMap::iterator tracer_lookup) {
  TRACE("CTracer::SwitchToZombieMode {}", THIS);

  auto raw_object = tracer_lookup->first;

  // this will resurrect the V8 object and keep it alive until we kill it
  tracer_lookup->second.m_v8_wrapper.ClearWeak();
  tracer_lookup->second.m_v8_wrapper.AnnotateStrongRetainer("naga.Tracer");

  // ask for a weakref to the Python object
  // TODO: investigate - creating python callables dynamically might be expensive
  // note both captured tracer and raw_object (PyObject*) a guaranteed to outlive this callable object
  py::cpp_function py_weak_ref_callback([this, raw_object = raw_object](const py::handle& py_weak_ref) {
    TRACE("CTracer::PythonWeakRefCallback py_weak_ref={} raw_object={}", py_weak_ref, raw_object);
    DeleteRecord(raw_object);
  });
  auto raw_weak_ref = PyWeakref_NewRef(raw_object, py_weak_ref_callback.ptr());
  assert(raw_weak_ref);

  // update our records
  assert(!tracer_lookup->second.m_weak_ref);
  tracer_lookup->second.m_weak_ref = raw_weak_ref;

  // stop holding the Python object strongly
  // we will be called back via a callback when the Python object is about to die
  Py_DECREF(raw_object);
}

void CTracer::AssociatedWrapperObjectIsAboutToDie(TracedRawObject* raw_object) {
  TRACE("CTracer::AssociatedWrapperObjectIsAboutToDie {} raw_object={}", THIS, raw_object);

  if (Py_REFCNT(raw_object) == 1) {
    // this is a fast path
    // if we are the last one holding the Python object
    // we can go ahead and let the V8 object die and release the Python object directly
    DeleteRecord(raw_object);
    Py_DECREF(raw_object);
  } else {
    // if there is someone else still holding the Python object
    // we want to keep our V8 object alive until the Python object goes away
    // we switch our Python object to weak mode and observe its release
    SwitchToZombieModeOrDie(raw_object);
  }
}