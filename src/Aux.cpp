#include "Aux.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kAuxLogger), __VA_ARGS__)

// this is useful when one wants to place a breakpoint to all changes to refcount of specified object
// in python test: print(STPyV8.refcount_addr(o)) and observe printed address
// e.g. in LLDB console, you can set a watchpoint via `w s e -- 0x123456`
py::str refCountAddr(py::object py_obj) {
  PyObject* raw_obj = py_obj.ptr();
  auto s = fmt::format("{}", static_cast<void*>(&raw_obj->ob_refcnt));
  TRACE("refCountAddr py_obj={} => {}", py_obj, s);
  return py::str(s);
}

// these functions are useful for conditionally enabling breakpoints at given trigger points
void trigger1() {
  TRACE("trigger1");
}

void trigger2() {
  TRACE("trigger2");
}

void trigger3() {
  TRACE("trigger3");
}

void trigger4() {
  TRACE("trigger4");
}

void trigger5() {
  TRACE("trigger5");
}

void trace(const py::str& s) {
  TRACE("trace: {}", s);
}

void v8RequestGarbageCollectionForTesting() {
  TRACE("v8Cleanup requested");
  auto v8_isolate = v8u::getCurrentIsolate();
  v8_isolate->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
  TRACE("v8Cleanup done");
}