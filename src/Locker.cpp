#include "Locker.h"

#include <memory>
#include "Isolate.h"
#include "PythonAllowThreadsGuard.h"

void CLocker::enter() {
  withPythonAllowThreadsGuard([&]() {
    auto v8_isolate = m_isolate.get() ? m_isolate->GetIsolate() : v8::Isolate::GetCurrent();
    m_v8_locker = std::make_unique<v8::Locker>(v8_isolate);
  });
}

void CLocker::leave() {
  withPythonAllowThreadsGuard([&]() { m_v8_locker.reset(); });
}

bool CLocker::IsLocked() {
  return v8::Locker::IsLocked(v8::Isolate::GetCurrent());
}

bool CLocker::IsActive() {
  return v8::Locker::IsActive();
}

void CLocker::Expose(const py::module& py_module) {
  // clang-format off
  py::class_<CLocker>(py_module, "JSLocker")
      .def(py::init<>())
      .def(py::init<CIsolatePtr>(), py::arg("isolate"))

      .def_property_readonly_static(
          "active", [](const py::object&) { return CLocker::IsActive(); },
          "whether Locker is being used by this V8 instance.")
      .def_property_readonly_static(
          "locked", [](const py::object&) { return CLocker::IsLocked(); },
          "whether or not the locker is locked by the current thread.")

      .def("entered", &CLocker::entered)
      .def("enter", &CLocker::enter)
      .def("leave", &CLocker::leave);
  // clang-format on
}
