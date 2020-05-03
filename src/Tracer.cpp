#include "Tracer.h"
#include "JSIsolate.h"
#include "Logging.h"
#include "Printing.h"
#include "PythonUtils.h"
#include "JSEternals.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kTracerLogger), __VA_ARGS__)

#define PyType_SUPPORTS_WEAKREFS(t) ((t)->tp_weaklistoffset > 0)

static PyObject* tracerPythonWeakCallback(PyObject* raw_self, PyObject* raw_weak_ref) {
  TRACE("tracerPythonWeakCallback raw_self={} raw_weak_ref={}", raw_self, raw_weak_ref);
  auto py_self = py::cast<py::capsule>(raw_self);
  CTracer* tracer = py_self;
  assert(tracer);
  tracer->WeakRefCallback(raw_weak_ref);
  Py_RETURN_NONE;
}

static PyMethodDef g_callback_def = {"tracerPythonWeakCallback", static_cast<PyCFunction>(tracerPythonWeakCallback),
                                     METH_O};

void traceWrapper(TracedRawObject* raw_object, v8::Local<v8::Object> v8_wrapper) {
  auto v8_isolate = v8_wrapper->GetIsolate();
  TRACE("traceWrapper v8_isolate={} raw_object={} v8_wrapper={}", P$(v8_isolate), py::handle(raw_object), v8_wrapper);
  auto isolate = CJSIsolate::FromV8(v8_isolate);
  isolate->Tracer().TraceWrapper(raw_object, v8_wrapper);
}

v8::Local<v8::Object> lookupTracedWrapper(v8::IsolatePtr v8_isolate, TracedRawObject* raw_object) {
  auto isolate = CJSIsolate::FromV8(v8_isolate);
  auto v8_result = isolate->Tracer().LookupWrapper(v8_isolate, raw_object);
  TRACE("lookupTracedWrapper v8_isolate={} raw_object={}", P$(v8_isolate), py::handle(raw_object), v8_result);
  return v8_result;
}

v8::Eternal<v8::Private> privateAPIForTracerPayload(v8::IsolatePtr v8_isolate) {
  return v8u::createEternalPrivateAPI(v8_isolate, "Naga#CTracer##payload");
}

TracedRawObject* lookupTracedObject(v8::Local<v8::Object> v8_wrapper) {
  TRACE("lookupTracedObject v8_wrapper={}", v8_wrapper);

  if (v8_wrapper.IsEmpty()) {
    return nullptr;
  }
  auto v8_isolate = v8_wrapper->GetIsolate();
  auto v8_context = v8u::getCurrentContext(v8_isolate);
  auto v8_payload_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kTracerPayload, privateAPIForTracerPayload);
  if (!v8_wrapper->HasPrivate(v8_context, v8_payload_api).ToChecked()) {
    return nullptr;
  }
  auto v8_val = v8_wrapper->GetPrivate(v8_context, v8_payload_api).ToLocalChecked();
  assert(!v8_val.IsEmpty());
  assert(v8_val->IsExternal());
  auto v8_payload = v8_val.As<v8::External>();
  auto raw_obj = static_cast<PyObject*>(v8_payload->Value());
  TRACE("lookupTracedObject => {}", S$(raw_obj));
  return raw_obj;
}

static void recordTracedWrapper(v8::Local<v8::Object> v8_wrapper, TracedRawObject* raw_object) {
  TRACE("recordTracedWrapper v8_wrapper={} raw_object={}", v8_wrapper, raw_object);
  auto v8_isolate = v8_wrapper->GetIsolate();
  auto v8_context = v8u::getCurrentContext(v8_isolate);
  auto v8_payload_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kTracerPayload, privateAPIForTracerPayload);
  auto v8_payload = v8::External::New(v8_isolate, raw_object);
  v8_wrapper->SetPrivate(v8_context, v8_payload_api, v8_payload);
}

static void v8WeakCallback(const v8::WeakCallbackInfo<TracedRawObject>& data) {
  // note that this call may come from anywhere, so we have to grab the GIL for potential Python API calls
  auto py_gil = pyu::withGIL();
  auto v8_isolate = data.GetIsolate();
  auto isolate = CJSIsolate::FromV8(v8_isolate);
  auto raw_object = data.GetParameter();
  TRACE("v8WeakCallback data.GetParameter={} v8_isolate={}", raw_object, P$(v8_isolate));
  isolate->Tracer().AssociatedWrapperObjectIsAboutToDie(raw_object);
}

// --------------------------------------------------------------------------------------------------------------------

CTracer::CTracer()
    : m_self(this, "Naga.Tracer"),
      m_callback(py::reinterpret_steal<py::object>(PyCFunction_NewEx(&g_callback_def, m_self.ptr(), nullptr))) {
  TRACE("CTracer::CTracer {} m_self={} m_callback={}", THIS, m_self, m_callback);
}

CTracer::~CTracer() {
  TRACE("CTracer::~CTracer {}", THIS);
  // make sure we release all Python objects in live mode and drop all pending weakrefs
  auto it = m_wrappers.begin();
  while (it != m_wrappers.end()) {
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
  assert(m_wrappers.find(raw_object) == m_wrappers.end());

  auto v8_isolate = v8_wrapper->GetIsolate();

  // add our record for lookups
  recordTracedWrapper(v8_wrapper, raw_object);
  auto insert_point =
      m_wrappers.insert(std::make_pair(raw_object, TracerRecord{V8Wrapper(v8_isolate, v8_wrapper), nullptr}));

  // start in live mode and we know we don't have to do any cleanup
  SwitchToLiveMode(insert_point.first, false);
}

v8::Local<v8::Object> CTracer::LookupWrapper(v8::IsolatePtr v8_isolate, PyObject* raw_object) {
  auto tracer_lookup = m_wrappers.find(raw_object);
  if (tracer_lookup == m_wrappers.end()) {
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

void CTracer::DeleteRecord(PyObject* dead_raw_object) {
  // WARNING! do not use dead_raw_object, even do not attempt to print it, it may be already gone
  TRACE("CTracer::DeleteRecord {} raw_object={}", THIS, static_cast<void*>(dead_raw_object));

  auto tracer_lookup = m_wrappers.find(dead_raw_object);
  assert(tracer_lookup != m_wrappers.end());

  auto& raw_weak_ref = tracer_lookup->second.m_weak_ref;
  if (raw_weak_ref) {
    assert(m_weak_refs.find(raw_weak_ref) != m_weak_refs.end());
    m_weak_refs.erase(raw_weak_ref);
    Py_DECREF(raw_weak_ref);
    raw_weak_ref = nullptr;
  }

  // remove our record
  // note that m_v8_wrapper.Reset() will be called in its destructor
  m_wrappers.erase(tracer_lookup);
}

void CTracer::SwitchToLiveMode(TrackedWrappers::iterator tracer_lookup, bool cleanup) {
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
    assert(m_weak_refs.find(raw_weak_ref) != m_weak_refs.end());
    m_weak_refs.erase(raw_weak_ref);
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
    TRACE("CTracer::SwitchToZombieMode weak reference is not possible for {}", Py_TYPE(raw_object));
    DeleteRecord(raw_object);
    Py_DECREF(raw_object);
    return;
  }

  auto tracer_lookup = m_wrappers.find(raw_object);
  assert(tracer_lookup != m_wrappers.end());

  SwitchToZombieMode(tracer_lookup);
}

void CTracer::WeakRefCallback(WeakRefRawObject* raw_weak_ref) {
  TRACE("CTracer::WeakRefCallback {} raw_weak_ref={}", THIS, raw_weak_ref);
  auto lookup = m_weak_refs.find(raw_weak_ref);
  assert(lookup != m_weak_refs.end());
  DeleteRecord(lookup->second);
}

void CTracer::SwitchToZombieMode(TrackedWrappers::iterator tracer_lookup) {
  auto raw_object = tracer_lookup->first;
  TRACE("CTracer::SwitchToZombieMode {} raw_object={}", THIS, raw_object);

  // this will resurrect the V8 object and keep it alive until we kill it
  tracer_lookup->second.m_v8_wrapper.ClearWeak();
  tracer_lookup->second.m_v8_wrapper.AnnotateStrongRetainer("naga.Tracer");

  // ask for a weakref to the Python object
  auto raw_weak_ref = PyWeakref_NewRef(raw_object, m_callback.ptr());
  assert(raw_weak_ref);
  m_weak_refs.insert(std::make_pair(raw_weak_ref, raw_object));

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