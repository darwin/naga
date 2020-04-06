#include "Locker.h"
#include "Isolate.h"
#include "PythonAllowThreadsGuard.h"

void CLocker::enter() {
  withPythonAllowThreadsGuard([&]() {
    auto isolate = m_isolate.get() ? m_isolate->GetIsolate() : v8::Isolate::GetCurrent();
    m_locker.reset(new v8::Locker(isolate));
  });
}

void CLocker::leave() {
  withPythonAllowThreadsGuard([&]() {
    m_locker.reset();
  });
}

bool CLocker::IsLocked() {
  return v8::Locker::IsLocked(v8::Isolate::GetCurrent());
}

bool CLocker::IsActive() {
  return v8::Locker::IsActive();
}

void CLocker::Expose(pb::module& m) {
  // clang-format off
  pb::class_<CLocker>(m, "JSLocker")
      .def(pb::init<>())
      .def(pb::init<CIsolatePtr>(), pb::arg("isolate"))

      .def_property_readonly_static(
          "active", [](pb::object) { return CLocker::IsActive(); },
          "whether Locker is being used by this V8 instance.")
      .def_property_readonly_static(
          "locked", [](pb::object) { return CLocker::IsLocked(); },
          "whether or not the locker is locked by the current thread.")

      .def("entered", &CLocker::entered)
      .def("enter", &CLocker::enter)
      .def("leave", &CLocker::leave);
  // clang-format on
}

void CUnlocker::enter() {
  withPythonAllowThreadsGuard([&]() {
    m_unlocker.reset(new v8::Unlocker(v8::Isolate::GetCurrent()));
  });
}

void CUnlocker::leave() {
  withPythonAllowThreadsGuard([&]() {
    m_unlocker.reset();
  });
}

void CUnlocker::Expose(pb::module& m) {
  // clang-format off
  pb::class_<CUnlocker>(m, "JSUnlocker")
      .def(pb::init<>())
      .def("entered", &CUnlocker::entered)
      .def("enter", &CUnlocker::enter)
      .def("leave", &CUnlocker::leave);
  // clang-format on
}
