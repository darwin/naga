#include "JSObjectBase.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

static bool isCLJSType(v8::Local<v8::Object> v8_obj) {
  if (v8_obj.IsEmpty()) {
    return false;
  }

  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  auto v8_ctor_key = v8::String::NewFromUtf8(v8_isolate, "constructor").ToLocalChecked();
  auto v8_ctor_val = v8_obj->Get(v8_context, v8_ctor_key).ToLocalChecked();

  if (v8_ctor_val.IsEmpty() || !v8_ctor_val->IsObject()) {
    return false;
  }

  auto v8_ctor_obj = v8::Local<v8::Object>::Cast(v8_ctor_val);
  auto v8_cljs_key = v8::String::NewFromUtf8(v8_isolate, "cljs$lang$type").ToLocalChecked();
  auto v8_cljs_val = v8_ctor_obj->Get(v8_context, v8_cljs_key).ToLocalChecked();

  return !(v8_cljs_val.IsEmpty() || !v8_cljs_val->IsBoolean());

  // note:
  // we should cast cljs_val to object and check cljs_obj->BooleanValue()
  // but we assume existence of property means true value
}

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

v8::Local<v8::Object> CJSObjectBase::Object() const {
  // TODO: isolate should be taken from the m_v8_obj
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_result = v8::Local<v8::Object>::New(v8_isolate, m_v8_obj);
  TRACE("CJSObjectBase::Object {} => {}", THIS, v8_result);
  return v8_result;
}

void CJSObjectBase::Dump(std::ostream& os) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  auto v8_obj = Object();
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
