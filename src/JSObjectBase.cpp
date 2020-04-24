#include "JSObjectBase.h"
#include "JSObjectUtils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

CJSObjectBase::CJSObjectBase(v8::Local<v8::Object> v8_obj)
    : m_roles(Roles::Generic), m_v8_obj(v8u::getCurrentIsolate(), v8_obj) {
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

bool CJSObjectBase::HasRole(Roles roles) const {
  return (m_roles & roles) == roles;
}

v8::Local<v8::Object> CJSObjectBase::Object(v8::IsolatePtr v8_isolate) const {
  auto v8_result = m_v8_obj.Get(v8_isolate);
  TRACE("CJSObjectBase::Object {} => {}", THIS, v8_result);
  return v8_result;
}

void CJSObjectBase::Dump(std::ostream& os) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  auto v8_obj = Object(v8_isolate);
  if (v8_obj.IsEmpty()) {
    os << "None";  // TODO: this should be something different than "None"
    return;
  }

  if (v8_obj->IsInt32()) {
    os << v8_obj->Int32Value(v8_context).ToChecked();
  } else if (v8_obj->IsNumber()) {
    os << v8_obj->NumberValue(v8_context).ToChecked();
  } else if (v8_obj->IsBoolean()) {
    os << v8_obj->BooleanValue(v8_isolate);
  } else if (v8_obj->IsNull()) {
    os << "None";
  } else if (v8_obj->IsUndefined()) {
    os << "N/A";
  } else if (v8_obj->IsString()) {
    os << *v8::String::Utf8Value(v8_isolate, v8::Local<v8::String>::Cast(v8_obj));
  } else {
    v8::MaybeLocal<v8::String> s = v8_obj->ToString(v8_context);
    if (s.IsEmpty()) {
      s = v8_obj->ObjectProtoToString(v8_context);
    } else {
      os << *v8::String::Utf8Value(v8_isolate, s.ToLocalChecked());
    }
  }
}
