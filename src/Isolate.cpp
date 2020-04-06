#include "Isolate.h"
#include "Context.h"

void CIsolate::Expose(pb::module& m) {
  // clang-format off
  pb::class_<CIsolate, CIsolatePtr>(m, "JSIsolate", "JSIsolate is an isolated instance of the V8 engine.")
      .def(pb::init<bool>(),
           pb::arg("owner") = false)

      .def_property_readonly_static(
          "current", [](pb::object) { return CIsolate::GetCurrent(); },
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

  v8::Isolate::CreateParams v8_create_params;
  v8_create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  m_v8_isolate = v8::Isolate::New(v8_create_params);
}

CIsolate::CIsolate(bool owner) {
  CIsolate::Init(owner);
}

CIsolate::CIsolate() {
  CIsolate::Init(false);
}

CIsolate::CIsolate(v8::Isolate* v8_isolate) : m_v8_isolate(v8_isolate), m_owner(false) {}

CIsolate::~CIsolate(void) {
  if (m_owner) {
    m_v8_isolate->Dispose();
  }
}

v8::Isolate* CIsolate::GetIsolate(void) {
  return m_v8_isolate;
}

CJavascriptStackTracePtr CIsolate::GetCurrentStackTrace(
    int frame_limit,
    v8::StackTrace::StackTraceOptions v8_options = v8::StackTrace::kOverview) {
  return CJavascriptStackTrace::GetCurrentStackTrace(m_v8_isolate, frame_limit, v8_options);
}

pb::object CIsolate::GetCurrent(void) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  if (!v8_isolate || !v8_isolate->IsInUse()) {
    return pb::none();
  }

  auto v8_scope = v8u::getScope(v8_isolate);
  return pb::cast(CIsolatePtr(new CIsolate(v8_isolate)));
}