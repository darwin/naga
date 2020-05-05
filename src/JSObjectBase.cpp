#include "JSObjectBase.h"
#include "JSObjectUtils.h"
#include "Logging.h"
#include "Printing.h"
#include "V8Utils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

CJSObjectBase::CJSObjectBase(v8::Local<v8::Object> v8_obj)
    : m_roles(Roles::Generic),
      m_v8_obj(v8u::getCurrentIsolate(), v8_obj) {
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
  TRACE("CJSObjectBase::CJSObjectBase {} v8_obj={} roles={}", THIS, v8_obj, m_roles);
}

CJSObjectBase::~CJSObjectBase() {
  TRACE("CJSObjectBase::~CJSObjectBase {}", THIS);
  m_v8_obj.Reset();
}

v8::Local<v8::Object> CJSObjectBase::ToV8(v8::IsolatePtr v8_isolate) const {
  auto v8_result = m_v8_obj.Get(v8_isolate);
  TRACE("CJSObjectBase::ToV8 {} => {}", THIS, v8_result);
  return v8_result;
}

bool CJSObjectBase::HasRole(Roles roles) const {
  return (m_roles & roles) == roles;
}

CJSObjectBase::Roles CJSObjectBase::GetRoles() const {
  return m_roles;
}

void CJSObjectBase::Dump(std::ostream& os) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_obj = ToV8(v8_isolate);
  if (v8_obj.IsEmpty()) {
    os << "<EMPTY>";
    return;
  } else {
    // generic "to string" conversion:
    //   Provide a string representation of this value usable for debugging.
    //   This operation has no observable side effects and will succeed
    //   unless e.g. execution is being terminated.
    auto v8_context = v8u::getCurrentContextUnchecked(v8_isolate);
    if (v8_context.IsEmpty()) {
      os << "<NO CONTEXT>";
      return;
    }
    auto v8_str = v8_obj->ToDetailString(v8_context).ToLocalChecked();
    auto v8_utf = v8u::toUTF(v8_isolate, v8_str);
    os << *v8_utf;
  }
}

std::ostream& operator<<(std::ostream& os, const CJSObjectBase::Roles& v) {
  std::vector<const char*> flags;
  flags.reserve(8);
  if ((v & CJSObjectBase::Roles::Function) == CJSObjectBase::Roles::Function) {
    flags.push_back("Function");
  }
  if ((v & CJSObjectBase::Roles::Array) == CJSObjectBase::Roles::Array) {
    flags.push_back("Array");
  }
  return os << fmt::format("{}", fmt::join(flags, ","));
}
