#include "JSIsolate.h"
#include "Tracer.h"
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

CJSIsolatePtr CJSIsolate::FromV8(v8::Isolate* v8_isolate) {
  auto isolate = lookupRegisteredIsolate(v8_isolate);
  if (!isolate) {
    auto msg = fmt::format("Cannot work with foreign V8 isolate {}", P$(v8_isolate));
    throw CJSException(v8x::ProtectedIsolatePtr(v8_isolate), msg);
  }
  TRACE("CJSIsolate::FromV8 v8_isolate={} => {}", P$(v8_isolate), (void*)isolate);
  return isolate->shared_from_this();
}

v8x::LockedIsolatePtr CJSIsolate::ToV8() {
  auto v8_isolate = m_v8_isolate.giveMeRawIsolateAndTrustMe();
  return v8x::LockedIsolatePtr(v8_isolate, m_locker_holder.CreateOrShareLocker());
}

CJSIsolate::CJSIsolate()
    : m_v8_isolate(v8x::createIsolate()),
      m_locker_holder(m_v8_isolate.giveMeRawIsolateAndTrustMe()),
      m_exposed_locker_level(0) {
  TRACE("CJSIsolate::CJSIsolate {}", THIS);
  m_eternals = std::make_unique<CJSEternals>(m_v8_isolate);
  m_tracer = std::make_unique<CTracer>();
  m_hospital = std::make_unique<CJSHospital>(m_v8_isolate);
  registerIsolate(m_v8_isolate, this);
}

CJSIsolate::~CJSIsolate() {
  TRACE("CJSIsolate::~CJSIsolate {}", THIS);

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
  m_v8_isolate.giveMeRawIsolateAndTrustMe()->Dispose();

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
  auto v8_isolate = m_v8_isolate.lock();
  return CJSStackTrace::GetCurrentStackTrace(v8_isolate, frame_limit, v8_options);
}

py::object CJSIsolate::GetCurrent() {
  auto v8_isolate_or_null = v8x::getCurrentIsolateUnchecked();
  auto py_result = [&] {
    if (!v8_isolate_or_null || !v8_isolate_or_null->IsInUse()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(FromV8(v8_isolate_or_null));
    }
  }();
  TRACE("CJSIsolate::GetCurrent => {}", py_result);
  return py_result;
}

void CJSIsolate::Enter() const {
  TRACE("CJSIsolate::Enter {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  v8_isolate->Enter();
}

void CJSIsolate::Leave() const {
  TRACE("CJSIsolate::Leave {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  v8_isolate->Exit();
}

py::object CJSIsolate::GetEnteredOrMicrotaskContext() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  auto py_result = [&] {
    if (v8_context.IsEmpty()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(CJSContext::FromV8(v8_context));
    }
  }();
  TRACE("CJSIsolate::GetEnteredOrMicrotaskContext {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSIsolate::GetCurrentContext() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContextUnchecked(v8_isolate);
  auto py_result = [&] {
    if (v8_context.IsEmpty()) {
      return py::js_null().cast<py::object>();
    } else {
      return py::cast(CJSContext::FromV8(v8_context));
    }
  }();
  TRACE("CJSIsolate::GetCurrentContext {} => {}", THIS, py_result);
  return py_result;
}

py::bool_ CJSIsolate::InContext() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto py_result = py::bool_(v8_isolate->InContext());
  TRACE("CJSIsolate::InContext {} => {}", THIS, py_result);
  return py_result;
}

// TODO: do not assert below, throw python runtime errors
void CJSIsolate::Lock() {
  TRACE("CJSIsolate::Lock {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_level >= 0);
  if (m_exposed_locker_level == 0) {
    m_exposed_locker = m_locker_holder.CreateOrShareLocker();
  }
  m_exposed_locker_level++;
}

void CJSIsolate::Unlock() {
  TRACE("CJSIsolate::Unlock {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_level > 0);
  m_exposed_locker_level--;
  if (m_exposed_locker_level == 0) {
    m_exposed_locker = nullptr;
  }
}

void CJSIsolate::UnlockAll() {
  TRACE("CJSIsolate::UnlockAll {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_level >= 0);
  m_exposed_locker_levels.push(m_exposed_locker_level);
  m_exposed_locker_level = 0;
  m_exposed_locker = nullptr;
}

void CJSIsolate::RelockAll() {
  assert(m_exposed_locker_level == 0);
  TRACE("CJSIsolate::RelockAll {} m_exposed_locker_level={}", THIS, m_exposed_locker_level);
  assert(m_exposed_locker_levels.size() > 0);
  m_exposed_locker_level = m_exposed_locker_levels.top();
  m_exposed_locker_levels.pop();
  if (m_exposed_locker_level > 0) {
    m_exposed_locker = m_locker_holder.CreateOrShareLocker();
  }
}

int CJSIsolate::LockLevel() const {
  return m_exposed_locker_level;
}

bool CJSIsolate::Locked() const {
  // this returns V8's opinion about locked state
  // please understand that the isolate could be locked because of m_exposed_locker (someone called JSIsolate.lock from
  // Python) or because some C++ code holds the lock themselves and this function happens to be called at that point.
  auto result = v8::Locker::IsLocked(m_v8_isolate.giveMeRawIsolateAndTrustMe());
  TRACE("CJSIsolate::Locked {} => {}", THIS, result);
  return result;
}
