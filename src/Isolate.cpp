#include "Isolate.h"

#include <utility>
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

CIsolate::CIsolate(bool owner) : m_v8_isolate(v8u::createIsolate()), m_owner(owner) {}

CIsolate::CIsolate() : m_v8_isolate(v8u::createIsolate()), m_owner(false) {}

CIsolate::CIsolate(v8::IsolateRef v8_isolate) : m_v8_isolate(std::move(v8_isolate)), m_owner(false) {}

CIsolate::~CIsolate() {
  if (m_owner) {
    m_v8_isolate->Dispose();
  }
}

v8::IsolateRef CIsolate::GetIsolate() {
  return m_v8_isolate;
}

CJSStackTracePtr CIsolate::GetCurrentStackTrace(
    int frame_limit,
    v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview) {
  return CJSStackTrace::GetCurrentStackTrace(m_v8_isolate, frame_limit, v8_options);
}

py::object CIsolate::GetCurrent() {
  // here we don't want to call our v8u::getCurrentIsolate, which returns non_)
  auto v8_nullable_isolate = v8::Isolate::GetCurrent();
  if (!v8_nullable_isolate || !v8_nullable_isolate->IsInUse()) {
    return py::none();
  }

  v8::IsolateRef v8_isolate(v8_nullable_isolate);
  auto v8_scope = v8u::openScope(v8_isolate);
  return py::cast(std::make_shared<CIsolate>(v8_isolate));
}