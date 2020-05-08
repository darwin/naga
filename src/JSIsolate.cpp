#include "JSIsolate.h"
#include "JSTracer.h"
#include "JSHospital.h"
#include "JSEternals.h"
#include "JSStackTrace.h"
#include "JSContext.h"
#include "JSException.h"
#include "JSIsolateRegistry.h"
#include "Logging.h"
#include "PybindExtensions.h"
#include "V8XProtectedIsolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSIsolateLogger), __VA_ARGS__)

SharedJSIsolatePtr JSIsolate::FromV8(v8::Isolate* v8_isolate) {
  auto isolate = lookupRegisteredIsolate(v8_isolate);
  if (!isolate) {
    auto msg = fmt::format("Cannot work with foreign V8 isolate {}", P$(v8_isolate));
    throw JSException(v8x::ProtectedIsolatePtr(v8_isolate), msg);
  }
  TRACE("JSIsolate::FromV8 v8_isolate={} => {}", P$(v8_isolate), (void*)isolate);
  return isolate->shared_from_this();
}

v8x::LockedIsolatePtr JSIsolate::ToV8() {
  return m_locker_holder.GetLockedIsolate();
}

JSIsolate::JSIsolate()
    : m_v8_isolate(v8x::createIsolate()),
      m_tracer(std::make_unique<decltype(m_tracer)::element_type>()),
      m_hospital(std::make_unique<decltype(m_hospital)::element_type>(m_v8_isolate)),
      m_eternals(std::make_unique<decltype(m_eternals)::element_type>(m_v8_isolate)),
      m_locker_holder(m_v8_isolate.giveMeRawIsolateAndTrustMe()),
      m_exposed_locker_level(0) {
  TRACE("JSIsolate::JSIsolate {}", THIS);
  registerIsolate(m_v8_isolate, this);
}

JSIsolate::~JSIsolate() {
  TRACE("JSIsolate::~JSIsolate {}", THIS);

  // hospital has to die and do cleanup before we call dispose
  m_hospital.reset();

  // tracer has to die and do cleanup before we call dispose
  m_tracer.reset();

  m_eternals.reset();

  unregisterIsolate(m_v8_isolate);

  assert(m_exposed_locker_level == 0);  // someone forgot to call unlock

  // This is interesting v8::Isolate::Dispose is documented to be used
  // under a lock but V8 asserts in debug mode.
  // It is probably a mistake in their header docs because the isolate is clearly trying to destroy all
  // associated mutexes.

  // below this point nobody should hold the lock
  auto v8_isolate = m_v8_isolate.giveMeRawIsolateAndTrustMe();
  assert(!v8_isolate->IsInUse());  // someone forgot to call leave
  v8_isolate->Dispose();

  TRACE("JSIsolate::~JSIsolate {} [COMPLETED]", THIS);
}

JSTracer& JSIsolate::Tracer() const {
  TRACE("JSIsolate::Tracer {} => {}", THIS, (void*)m_tracer.get());
  return *m_tracer.get();
}

JSHospital& JSIsolate::Hospital() const {
  TRACE("JSIsolate::Hospital {} => {}", THIS, (void*)m_hospital.get());
  return *m_hospital.get();
}

JSEternals& JSIsolate::Eternals() const {
  TRACE("JSIsolate::Eternals {} => {}", THIS, (void*)m_eternals.get());
  return *m_eternals.get();
}

SharedJSStackTracePtr JSIsolate::GetCurrentStackTrace(int frame_limit,
                                                      v8::StackTrace::StackTraceOptions v8_options) const {
  TRACE("JSIsolate::GetCurrentStackTrace {} frame_limit={} v8_options={:#x}", THIS, frame_limit, v8_options);
  auto v8_isolate = m_v8_isolate.lock();
  return JSStackTrace::GetCurrentStackTrace(v8_isolate, frame_limit, v8_options);
}

py::object JSIsolate::GetCurrent() {
  auto v8_isolate_or_null = v8x::getCurrentIsolateUnchecked();
  auto py_result = [&] {
    if (!v8_isolate_or_null || !v8_isolate_or_null->IsInUse()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(FromV8(v8_isolate_or_null));
    }
  }();
  TRACE("JSIsolate::GetCurrent => {}", py_result);
  return py_result;
}

void JSIsolate::Enter() const {
  TRACE("JSIsolate::Enter {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  v8_isolate->Enter();
}

void JSIsolate::Leave() const {
  TRACE("JSIsolate::Leave {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  v8_isolate->Exit();
}

py::object JSIsolate::GetEnteredOrMicrotaskContext() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  auto py_result = [&] {
    if (v8_context.IsEmpty()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(JSContext::FromV8(v8_context));
    }
  }();
  TRACE("JSIsolate::GetEnteredOrMicrotaskContext {} => {}", THIS, py_result);
  return py_result;
}

py::object JSIsolate::GetCurrentContext() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContextUnchecked(v8_isolate);
  auto py_result = [&] {
    if (v8_context.IsEmpty()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(JSContext::FromV8(v8_context));
    }
  }();
  TRACE("JSIsolate::GetCurrentContext {} => {}", THIS, py_result);
  return py_result;
}

py::bool_ JSIsolate::InContext() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto py_result = py::bool_(v8_isolate->InContext());
  TRACE("JSIsolate::InContext {} => {}", THIS, py_result);
  return py_result;
}

// TODO: do not assert below, throw python runtime errors
void JSIsolate::Lock() {
  TRACE("JSIsolate::Lock {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_level >= 0);
  if (m_exposed_locker_level == 0) {
    m_exposed_locker = m_locker_holder.CreateOrShareLocker();
  }
  m_exposed_locker_level++;
}

void JSIsolate::Unlock() {
  TRACE("JSIsolate::Unlock {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_level > 0);
  m_exposed_locker_level--;
  if (m_exposed_locker_level == 0) {
    m_exposed_locker = nullptr;
  }
}

void JSIsolate::UnlockAll() {
  TRACE("JSIsolate::UnlockAll {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_level >= 0);
  m_exposed_locker_levels.push(m_exposed_locker_level);
  m_exposed_locker_level = 0;
  m_exposed_locker = nullptr;
}

void JSIsolate::RelockAll() {
  assert(m_exposed_locker_level == 0);
  TRACE("JSIsolate::RelockAll {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_levels.size() > 0);
  m_exposed_locker_level = m_exposed_locker_levels.top();
  m_exposed_locker_levels.pop();
  if (m_exposed_locker_level > 0) {
    m_exposed_locker = m_locker_holder.CreateOrShareLocker();
  }
}

int JSIsolate::LockLevel() const {
  return m_exposed_locker_level;
}

bool JSIsolate::Locked() const {
  // this returns V8's opinion about locked state
  // please understand that the isolate could be locked because of m_exposed_locker (someone called JSIsolate.lock from
  // Python) or because some C++ code holds the lock themselves and this function happens to be called at that point.
  auto result = v8::Locker::IsLocked(m_v8_isolate.giveMeRawIsolateAndTrustMe());
  TRACE("JSIsolate::Locked {} => {}", THIS, result);
  return result;
}
