#pragma once

#include "Base.h"

// This tracer functionality is for optimization.
// When there is a non-trivial Python object crossing the boundary into JS land we create a JS wrapper object for it.
// That is not cheap, and when this is happening repeatedly with the same object we would like to cache the wrapper.
// The question is how we should manage wrapper's life time.
// For example we must keep in mind that a reference to the wrapper could be held/used by someone in JS land
// so we must keep the Python object alive as long as the wrapper is alive (even if it would have been already
// released on Python side).
//
// Our approach here:
// 1. when a wrapper for Python object is requested we first try to lookup cached version (via lookupTracedWrapper)
//    and use it
// 2. otherwise we create a new wrapper and register it in our tracer cache (via traceWrapper) for future lookups
//
// Our cache holds pairs of <PyObject and associated V8 wrapper object> (m_tracked_objects)
// a) A newly added pair starts in "live mode" which means we strongly hold the PyObject and mark the wrapper object
//    as weak.
// b) When we get a callback from V8 that the wrapper object is about to die meaning that last V8-land person stopped
//    holding it - we switch to "zombie mode" which means that we start holding the wrapper strongly (resurrecting it
//    into a zombie) and start holding the Python object weakly, waiting for its release on Python side
// c) When in zombie mode and we get a callback from Python that the object is about to die, we remove the pair from
//    our cache (this will let the Python object to die and zombie wrapper will be killed as well)
// d) When in zombie mode and we observe a new lookup for Python object, we return the existing (resurrected zombie)
//    wrapper but we must switch to "live mode" again because the wrapper could stay held on JS side again.
//    That means we have to start holding the Python object strongly again and mark resurrected zombie as weak again.
//    Note that the zombie won't die instantly under our hands because we are returning a separate reference to it back
//    to a JS-land caller. Of course, if the caller drops it, we should get a v8WeakCallback at some later point
//    (depends on V8 GC mood).
//
// Please note a few edge cases:
// b1) When we are about to switch to "zombie mode" we can test that we are the last person holding the Python object.
//     In this case we can perform direct fast path and drop the pair immediately. No need to create Python weak ref.
// b2) Not all Python objects can have weak refs for some reason (unfortunately!), we detect this situation and
//     in this (hopefully) rare case we simply drop the pair from the cache. This is not optimal because next time
//     the wrapper will have to be re-created but there is nothing much we can do without a weak reference support.
//     Ideas?
// b3) We rely on V8's callbacks which are not guaranteed. If V8 does not call us soon (or at all) it is not critical.
//     The result is that we keep the pairs in the cache longer than we would had otherwise.
//     This is a little tricky for testing because these callbacks may be non-deterministic.
//     If you need to force garbage collection for Python tests, use `v8_request_gc_for_testing`.
//     Also note that each tracer is owned by some isolate and it gets a chance to do cleanup before isolate
//     goes away. So there won't be any leaks after isolate gets destroyed.
//
// What about the other direction?
//
// As you can see this functionality is related to wrapping Python objects when crossing into JS land.
// The other direction is handled by pybind11 library. The library is responsible for creating Python wrappers of
// JS objects (C++ objects) when crossing into Python land.
// Internally they have similar mechanism which keeps track of wrappers created and keeps them alive as needed.
// I'm not sure how this works exactly and if the wrappers generation/usage is optimal with our code.
// TODO: learn more
//
// There is definitely one case when we have to assist. We have to prevent double-wrapping.
// We want to detect cases when a JS wrapper is crossing the boundary back into Python land (lookupTracedObject).
// If the the JS object is a wrapper, we simply use this cache to get the original naked Python object.
// We must avoid wrapping the wrapper!

typedef v8::Global<v8::Object> V8Wrapper;
typedef PyObject TracedRawObject;
typedef PyObject WeakRefRawObject;

void traceWrapper(TracedRawObject* raw_object, v8::Local<v8::Object> v8_wrapper);
v8::Local<v8::Object> lookupTracedWrapper(v8::IsolateRef v8_isolate, TracedRawObject* raw_object);
TracedRawObject* lookupTracedObject(v8::Local<v8::Object> v8_wrapper);

struct TracerRecord {
  V8Wrapper m_v8_wrapper;
  WeakRefRawObject* m_weak_ref;  // this field is non-null when in zombie mode
};

typedef std::map<TracedRawObject*, TracerRecord> WrapperTrackingMap;

class CTracer {
  WrapperTrackingMap m_tracked_wrappers;

 public:
  CTracer();
  ~CTracer();

  void TraceWrapper(TracedRawObject* raw_object, v8::Local<v8::Object> v8_wrapper);
  v8::Local<v8::Object> LookupWrapper(v8::IsolateRef v8_isolate, TracedRawObject* raw_object);
  void AssociatedWrapperObjectIsAboutToDie(TracedRawObject* raw_object);

 protected:
  void DeleteRecord(TracedRawObject* raw_object);
  void SwitchToLiveMode(WrapperTrackingMap::iterator tracer_lookup, bool cleanup = true);
  void SwitchToZombieMode(WrapperTrackingMap::iterator tracer_lookup);
  void SwitchToZombieModeOrDie(TracedRawObject* raw_object);
};