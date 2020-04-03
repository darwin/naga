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
  withPythonAllowThreadsGuard([&]() { m_locker.reset(); });
}

bool CLocker::IsLocked() {
  return v8::Locker::IsLocked(v8::Isolate::GetCurrent());
}

void CLocker::Expose() {
  py::class_<CLocker, boost::noncopyable>("JSLocker", py::no_init)
      .def(py::init<>())
      .def(py::init<CIsolatePtr>((py::arg("isolate"))))

      .add_static_property("active", &v8::Locker::IsActive, "whether Locker is being used by this V8 instance.")

      .add_static_property("locked", &CLocker::IsLocked, "whether or not the locker is locked by the current thread.")

      .def("entered", &CLocker::entered)
      .def("enter", &CLocker::enter)
      .def("leave", &CLocker::leave);

  py::class_<CUnlocker, boost::noncopyable>("JSUnlocker")
      .def("entered", &CUnlocker::entered)
      .def("enter", &CUnlocker::enter)
      .def("leave", &CUnlocker::leave);
}

void CUnlocker::enter() {
  withPythonAllowThreadsGuard([&]() { m_unlocker.reset(new v8::Unlocker(v8::Isolate::GetCurrent())); });
}

void CUnlocker::leave() {
  withPythonAllowThreadsGuard([&]() { m_unlocker.reset(); });
}
