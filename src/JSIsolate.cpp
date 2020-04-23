#include "JSIsolate.h"
#include "Tracer.h"
#include "JSHospital.h"
#include "JSEternals.h"
#include "JSStackTrace.h"
#include "JSNull.h"
#include "JSContext.h"
#include "JSException.h"
#include "JSIsolateRegistry.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSIsolateLogger), __VA_ARGS__)

CJSIsolatePtr CJSIsolate::FromV8(const v8::IsolateRef& v8_isolate) {
  auto isolate = lookupRegisteredIsolate(v8_isolate);
  if (!isolate) {
    throw CJSException(v8_isolate, fmt::format("Cannot work with foreign V8 isolate {}", P$(v8_isolate)));
  }
  TRACE("CJSIsolate::FromV8 v8_isolate={} => {}", P$(v8_isolate), (void*)isolate);
  return isolate->shared_from_this();
}

CJSIsolate::CJSIsolate() : m_v8_isolate(v8u::createIsolate()) {
  TRACE("CJSIsolate::CJSIsolate {}", THIS);
  m_eternals = std::make_unique<CJSEternals>(m_v8_isolate);
  m_tracer = std::make_unique<CTracer>();
  m_hospital = std::make_unique<CJSHospital>(m_v8_isolate);
  registerIsolate(m_v8_isolate, this);
}

CJSIsolate::~CJSIsolate() {
  TRACE("CJSIsolate::~CJSIsolate {}", THIS);

  unregisterIsolate(m_v8_isolate);

  // isolate could be entered, we cannot dispose unless we exit it completely
  int defensive_counter = 0;
  while (m_v8_isolate->IsInUse()) {
    Leave();
    if (++defensive_counter > 100) {
      break;
    }
  }

  // hospital has to die and do cleanup before we call dispose
  m_hospital.reset();

  // tracer has to die and do cleanup before we call dispose
  m_tracer.reset();

  m_eternals.reset();

  Dispose();
  TRACE("CJSIsolate::~CJSIsolate {} [COMPLETED]", THIS);
}

CTracer& CJSIsolate::Tracer() const {
  TRACE("CJSIsolate::Tracer {} => {}", THIS, (void*)m_tracer.get());
  return *m_tracer.get();
}

CJSHospital& CJSIsolate::Hospital() const {
  TRACE("CJSIsolate::Hospital {} => {}", THIS, (void*)m_hospital.get());
  return *m_hospital.get();
}

CJSEternals& CJSIsolate::Eternals() const {
  TRACE("CJSIsolate::Eternals {} => {}", THIS, (void*)m_eternals.get());
  return *m_eternals.get();
}

CJSStackTracePtr CJSIsolate::GetCurrentStackTrace(int frame_limit, v8::StackTrace::StackTraceOptions v8_options) const {
  TRACE("CJSIsolate::GetCurrentStackTrace {} frame_limit={} v8_options={:#x}", THIS, frame_limit, v8_options);
  return CJSStackTrace::GetCurrentStackTrace(m_v8_isolate, frame_limit, v8_options);
}

py::object CJSIsolate::GetCurrent() {
  // here we don't want to call our v8u::getCurrentIsolate, which returns not_null<>)
  auto v8_nullable_isolate = v8::Isolate::GetCurrent();
  auto py_result = ([&] {
    if (!v8_nullable_isolate || !v8_nullable_isolate->IsInUse()) {
      return py::js_null().cast<py::object>();
    } else {
      v8::IsolateRef v8_isolate(v8_nullable_isolate);
      return py::cast(FromV8(v8_isolate));
    }
  })();
  TRACE("CJSIsolate::GetCurrent => {}", py_result);
  return py_result;
}

bool CJSIsolate::IsLocked() const {
  auto result = v8::Locker::IsLocked(m_v8_isolate);
  TRACE("CJSIsolate::IsLocked {} => {}", THIS, result);
  return result;
}

void CJSIsolate::Enter() const {
  TRACE("CJSIsolate::Enter {}", THIS);
  m_v8_isolate->Enter();
}

void CJSIsolate::Leave() const {
  TRACE("CJSIsolate::Leave {}", THIS);
  m_v8_isolate->Exit();
}

void CJSIsolate::Dispose() const {
  TRACE("CJSIsolate::Dispose {}", THIS);
  m_v8_isolate->Dispose();
}

py::object CJSIsolate::GetEnteredOrMicrotaskContext() const {
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto v8_context = m_v8_isolate->GetEnteredOrMicrotaskContext();
  auto py_result = ([&] {
    if (v8_context.IsEmpty()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(CJSContext::FromV8(v8_context));
    }
  })();
  TRACE("CJSIsolate::GetEnteredOrMicrotaskContext {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSIsolate::GetCurrentContext() const {
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto v8_context = m_v8_isolate->GetCurrentContext();
  auto py_result = ([&] {
    if (v8_context.IsEmpty()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(CJSContext::FromV8(v8_context));
    }
  })();
  TRACE("CJSIsolate::GetCurrentContext {} => {}", THIS, py_result);
  return py_result;
}

py::bool_ CJSIsolate::InContext() const {
  auto v8_scope = v8u::withScope(m_v8_isolate);
  auto py_result = py::bool_(m_v8_isolate->InContext());
  TRACE("CJSIsolate::InContext {} => {}", THIS, py_result);
  return py_result;
}