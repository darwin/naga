#include "Aux.h"
#include "JSIsolate.h"
#include "JSContext.h"
#include "Logging.h"
#include "V8Utils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kAuxLogger), __VA_ARGS__)

// this is useful when one wants to place a breakpoint to all changes to refcount of specified object
// in python test: print(naga.aux.refcount_addr(o)) and observe printed address
// e.g. in LLDB console, you can set a watchpoint via `w s e -- 0x123456`
py::str refCountAddr(const py::object& py_obj) {
  auto raw_obj = py_obj.ptr();
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

CJSIsolatePtr testEncounteringForeignIsolate() {
  auto foreign_v8_isolate = v8u::createIsolate();
  return CJSIsolate::FromV8(foreign_v8_isolate);
}

CJSContextPtr testEncounteringForeignContext() {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto foreign_v8_context = v8::Context::New(v8_isolate);
  return CJSContext::FromV8(foreign_v8_context);
}

py::str getCurrentStackTrace() {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto stackTraceText = v8u::getCurrentStackTrace(v8_isolate);
  TRACE("getCurrentStackTrace => {}", traceText(stackTraceText));
  return py::str(stackTraceText);
}
