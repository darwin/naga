#include "Locker.h"

#include <memory>
#include "Isolate.h"
#include "PythonAllowThreadsGuard.h"

#define TRACE(...) RAII_LOGGER_INDENT; SPDLOG_LOGGER_TRACE(getLogger(kLockingLogger), __VA_ARGS__)

void CLocker::Expose(const py::module& py_module) {
  TRACE("CLocker::Expose py_module={}", py_module);
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

      .def("entered", &CLocker::IsEntered)
      .def("enter", &CLocker::Enter)
      .def("leave", &CLocker::Leave);
  // clang-format on
}

CLocker::CLocker(CIsolatePtr isolate) : m_isolate(std::move(isolate)) {
  TRACE("CLocker::CLocker {} isolate={}", THIS, m_isolate);
}

bool CLocker::IsEntered() {
  auto result = (bool)m_v8_locker.get();
  TRACE("CLocker::IsEntered {} => {}", THIS, result);
  return result;
}

void CLocker::Enter() {
  TRACE("CLocker::Enter {}", THIS);
  withPythonAllowThreadsGuard([&]() {
    auto v8_isolate = m_isolate.get() ? m_isolate->GetIsolate() : v8u::getCurrentIsolate();
    m_v8_locker = std::make_unique<v8::Locker>(v8_isolate);
  });
}

void CLocker::Leave() {
  TRACE("CLocker::Leave {}", THIS);
  withPythonAllowThreadsGuard([&]() { m_v8_locker.reset(); });
}

bool CLocker::IsLocked() {
  auto result = v8::Locker::IsLocked(v8u::getCurrentIsolate());
  TRACE("CLocker::IsLocked => {}", result);
  return result;
}

bool CLocker::IsActive() {
  auto result = v8::Locker::IsActive();
  TRACE("CLocker::IsActive => {}", result);
  return result;
}
