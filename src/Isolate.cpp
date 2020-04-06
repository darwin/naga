#include "Isolate.h"
#include "Context.h"
#include "JSStackTrace.h"

void CIsolate::Expose(const py::module& py_module) {
  // clang-format off
  py::class_<CIsolate, CIsolatePtr>(py_module, "JSIsolate", "JSIsolate is an isolated instance of the V8 engine.")
      .def(py::init<bool>(),
           py::arg("owner") = false)

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

void CIsolate::Init(bool owner) {
  m_owner = owner;

  if (!m_v8_isolate) {
    v8::Isolate::CreateParams v8_create_params;
    v8_create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    m_v8_isolate = v8::Isolate::New(v8_create_params);
  }
}

CIsolate::CIsolate(bool owner) : m_v8_isolate(nullptr), m_owner(false) {
  CIsolate::Init(owner);
}

CIsolate::CIsolate() : m_v8_isolate(nullptr), m_owner(false) {
  CIsolate::Init(false);
}

CIsolate::CIsolate(v8::Isolate* v8_isolate) : m_v8_isolate(v8_isolate), m_owner(false) {
  CIsolate::Init(false);
}

CIsolate::~CIsolate() {
  if (m_owner) {
    m_v8_isolate->Dispose();
  }
}

v8::Isolate* CIsolate::GetIsolate() {
  return m_v8_isolate;
}

CJSStackTracePtr CIsolate::GetCurrentStackTrace(
    int frame_limit,
    v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview) {
  return CJSStackTrace::GetCurrentStackTrace(m_v8_isolate, frame_limit, v8_options);
}

py::object CIsolate::GetCurrent() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate || !v8_isolate->IsInUse()) {
    return py::none();
  }

  auto v8_scope = v8u::getScope(v8_isolate);
  return py::cast(CIsolatePtr(new CIsolate(v8_isolate)));
}