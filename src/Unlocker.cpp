#include "Unlocker.h"
#include "PythonAllowThreadsGuard.h"

void CUnlocker::enter() {
  withPythonAllowThreadsGuard([&]() { m_v8_unlocker = std::make_unique<v8::Unlocker>(v8u::getCurrentIsolate()); });
}

void CUnlocker::leave() {
  withPythonAllowThreadsGuard([&]() { m_v8_unlocker.reset(); });
}

void CUnlocker::Expose(const py::module& py_module) {
  // clang-format off
  py::class_<CUnlocker>(py_module, "JSUnlocker")
      .def(py::init<>())
      .def("entered", &CUnlocker::entered)
      .def("enter", &CUnlocker::enter)
      .def("leave", &CUnlocker::leave);
  // clang-format on
}
