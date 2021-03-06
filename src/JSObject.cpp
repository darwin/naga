#include "JSObject.h"
#include "JSObjectUtils.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

JSObject::JSObject(v8::Local<v8::Object> v8_obj) : m_roles(Roles::Generic), m_v8_obj(v8x::getCurrentIsolate(), v8_obj) {
  m_v8_obj.AnnotateStrongRetainer("Naga JSObject");

  // detect supported object roles
  if (v8_obj->IsFunction()) {
    m_roles |= Roles::Function;
  }
  if (v8_obj->IsArray()) {
    m_roles |= Roles::Array;
  }
#ifdef NAGA_FEATURE_CLJS
  if (isCLJSType(v8_obj)) {
    m_roles |= Roles::CLJS;
  }
#endif
  TRACE("JSObject::JSObject {} v8_obj={} roles={}", THIS, v8_obj, m_roles);
}

JSObject::~JSObject() {
  TRACE("JSObject::~JSObject {}", THIS);
  m_v8_obj.Reset();
}

v8::Local<v8::Object> JSObject::ToV8(v8x::LockedIsolatePtr& v8_isolate) const {
  auto v8_result = m_v8_obj.Get(v8_isolate);
  TRACE("JSObject::ToV8 {} => {}", THIS, v8_result);
  return v8_result;
}

bool JSObject::HasRole(Roles roles) const {
  return (m_roles & roles) == roles;
}

JSObject::Roles JSObject::GetRoles() const {
  return m_roles;
}

void JSObject::Dump(std::ostream& os) const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto v8_obj = ToV8(v8_isolate);
  if (v8_obj.IsEmpty()) {
    os << "<EMPTY>";
    return;
  } else {
    // generic "to string" conversion:
    //   Provide a string representation of this value usable for debugging.
    //   This operation has no observable side effects and will succeed
    //   unless e.g. execution is being terminated.
    auto v8_context = v8x::getCurrentContextUnchecked(v8_isolate);
    if (v8_context.IsEmpty()) {
      os << "<NO CONTEXT>";
      return;
    }
    auto v8_str = v8_obj->ToDetailString(v8_context).ToLocalChecked();
    auto v8_utf = v8x::toUTF(v8_isolate, v8_str);
    os << *v8_utf;
  }
}

std::ostream& operator<<(std::ostream& os, const JSObject::Roles& v) {
  std::vector<const char*> flags;
  flags.reserve(8);
  if ((v & JSObject::Roles::Function) == JSObject::Roles::Function) {
    flags.push_back("Function");
  }
  if ((v & JSObject::Roles::Array) == JSObject::Roles::Array) {
    flags.push_back("Array");
  }
  return os << fmt::format("{}", fmt::join(flags, ","));
}
