#include "Locker.h"
#include "Isolate.h"
#include "PythonAllowThreadsGuard.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kLockingLogger), __VA_ARGS__)

void CLocker::Expose(py::module py_module) {
  TRACE("CLocker::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CLocker>(py_module, "JSLocker")
      .def(py::init<>())

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

bool CLocker::IsEntered() {
  auto result = static_cast<bool>(m_v8_locker.get());
  TRACE("CLocker::IsEntered {} => {}", THIS, result);
  return result;
}

void CLocker::Enter() {
  TRACE("CLocker::Enter {}", THIS);
  withPythonAllowThreadsGuard([&]() {
    auto v8_isolate = v8u::getCurrentIsolate();
    m_isolate = CIsolate::FromV8(v8_isolate);
    m_v8_locker = std::make_unique<v8::Locker>(v8_isolate);
  });
}

void CLocker::Leave() {
  TRACE("CLocker::Leave {}", THIS);
  withPythonAllowThreadsGuard([&]() {
    m_v8_locker.reset();
    m_isolate.reset();
  });
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
