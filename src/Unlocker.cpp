#include "Unlocker.h"
#include "PythonAllowThreadsGuard.h"

#define TRACE(...) (SPDLOG_LOGGER_TRACE(getLogger(kLockingLogger), __VA_ARGS__))

void CUnlocker::Expose(const py::module& py_module) {
  TRACE("CUnlocker::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CUnlocker>(py_module, "JSUnlocker")
      .def(py::init<>())
      .def("entered", &CUnlocker::entered)
      .def("enter", &CUnlocker::enter)
      .def("leave", &CUnlocker::leave);
  // clang-format on
}

bool CUnlocker::entered() {
  auto result = (bool)m_v8_unlocker.get();
  TRACE("CUnlocker::entered {} => {}", THIS, result);
  return result;
}

void CUnlocker::enter() {
  TRACE("CUnlocker::enter {}", THIS);
  withPythonAllowThreadsGuard([&]() { m_v8_unlocker = std::make_unique<v8::Unlocker>(v8u::getCurrentIsolate()); });
}

void CUnlocker::leave() {
  TRACE("CUnlocker::leave {}", THIS);
  withPythonAllowThreadsGuard([&]() { m_v8_unlocker.reset(); });
}
