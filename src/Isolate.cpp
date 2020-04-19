#include "Isolate.h"

#include "Context.h"
#include "Tracer.h"
#include "JSStackTrace.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kIsolateLogger), __VA_ARGS__)

const int kSelfDataSlotIndex = 0;

void CIsolate::Expose(py::module py_module) {
  TRACE("CIsolate::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CIsolate, CIsolatePtr>(py_module, "JSIsolate", "JSIsolate is an isolated instance of the V8 engine.")
      .def(py::init<>())

      .def_property_readonly_static(
          "current", [](const py::object&) { return CIsolate::GetCurrent(); },
          "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")

      .def_property_readonly("locked", &CIsolate::IsLocked)

      .def("GetCurrentStackTrace", &CIsolate::GetCurrentStackTrace)

      .def("enter", &CIsolate::Enter,
           "Sets this isolate as the entered one for the current thread. "
           "Saves the previously entered one (if any), so that it can be "
           "restored when exiting.  Re-entering an isolate is allowed.")

      .def("leave", &CIsolate::Leave,
           "Exits this isolate by restoring the previously entered one in the current thread. "
           "The isolate may still stay the same, if it was entered more than once.");
  // clang-format on
}

CIsolatePtr CIsolate::FromV8(const v8::IsolateRef& v8_isolate) {
  // FromV8 may be called only on isolates created by our constructor
  assert(v8_isolate->GetNumberOfDataSlots() > kSelfDataSlotIndex);
  auto isolate_ptr = static_cast<CIsolate*>(v8_isolate->GetData(kSelfDataSlotIndex));
  assert(isolate_ptr);
  TRACE("CIsolate::FromV8 v8_isolate={} => {}", isolateref_printer{v8_isolate}, (void*)isolate_ptr);
  return isolate_ptr->shared_from_this();
}

CIsolate::CIsolate() : m_v8_isolate(v8u::createIsolate()) {
  TRACE("CIsolate::CIsolate {}", THIS);
  m_tracer = new CTracer();
  m_v8_isolate->SetData(kSelfDataSlotIndex, this);
}

CIsolate::~CIsolate() {
  TRACE("CIsolate::~CIsolate {}", THIS);

  // tracer has to die and do cleanup before we call dispose
  delete m_tracer;
  m_tracer = nullptr;

  // isolate could be entered, we cannot dispose unless we exit it completely
  while (m_v8_isolate->IsInUse()) {
    Leave();
  }
  Dispose();
  TRACE("CIsolate::~CIsolate {} [COMPLETED]", THIS);
}

CJSStackTracePtr CIsolate::GetCurrentStackTrace(int frame_limit, v8::StackTrace::StackTraceOptions v8_options) {
  TRACE("CIsolate::GetCurrentStackTrace {} frame_limit={} v8_options={:#x}", THIS, frame_limit, v8_options);
  return CJSStackTrace::GetCurrentStackTrace(m_v8_isolate, frame_limit, v8_options);
}

py::object CIsolate::GetCurrent() {
  TRACE("CIsolate::GetCurrent");
  // here we don't want to call our v8u::getCurrentIsolate, which returns not_null<>)
  auto v8_nullable_isolate = v8::Isolate::GetCurrent();
  if (!v8_nullable_isolate || !v8_nullable_isolate->IsInUse()) {
    TRACE("CIsolate::GetCurrent => None");
    return py::none();
  }

  v8::IsolateRef v8_isolate(v8_nullable_isolate);
  return py::cast(FromV8(v8_isolate));
}

bool CIsolate::IsLocked() {
  auto result = v8::Locker::IsLocked(m_v8_isolate);
  TRACE("CIsolate::IsLocked {} => {}", THIS, result);
  return result;
}

void CIsolate::Enter() {
  TRACE("CIsolate::Enter {}", THIS);
  m_v8_isolate->Enter();
}

void CIsolate::Leave() {
  TRACE("CIsolate::Leave {}", THIS);
  m_v8_isolate->Exit();
}

void CIsolate::Dispose() {
  TRACE("CIsolate::Dispose {}", THIS);
  m_v8_isolate->Dispose();
}

CTracer* CIsolate::Tracer() {
  TRACE("CIsolate::Tracer {} => {}", THIS, (void*)m_tracer);
  return m_tracer;
}
