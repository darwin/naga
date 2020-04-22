#include "JSObjectBase.h"
#include "JSUndefined.h"
#include "JSNull.h"
#include "PythonDateTime.h"
#include "Tracer.h"

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
  m_v8_obj.AnnotateStrongRetainer("Naga CJSObject");

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

py::object CJSObjectBase::Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this) {
  TRACE("CJSObjectBase::Wrap v8_isolate={} v8_val={} v8_this={}", P$(v8_isolate), v8_val, v8_this);

  if (v8_val->IsFunction()) {
    // when v8_this is global, we can fall back to unbound function (later)
    auto v8_context = v8_isolate->GetCurrentContext();
    if (!v8_this->StrictEquals(v8_context->Global())) {
      // TODO: optimize this
      auto v8_fn = v8_val.As<v8::Function>();
      auto v8_bind_key = v8::String::NewFromUtf8(v8_isolate, "bind").ToLocalChecked();
      auto v8_bind_val = v8_fn->Get(v8_context, v8_bind_key).ToLocalChecked();
      assert(v8_bind_val->IsFunction());
      auto v8_bind_fn = v8_bind_val.As<v8::Function>();
      v8::Local<v8::Value> args[] = {v8_this};
      auto v8_bound_val = v8_bind_fn->Call(v8_context, v8_fn, std::size(args), args).ToLocalChecked();
      assert(v8_bound_val->IsFunction());
      auto v8_bound_fn = v8_bound_val.As<v8::Function>();
      TRACE("CJSObjectBase::Wrap v8_bound_fn={}", v8_bound_fn);
      // TODO: mark as function role?
      return Wrap(v8_isolate, std::make_shared<CJSObject>(v8_bound_fn));
    }
  }

  return Wrap(v8_isolate, v8_val);
}

py::object CJSObjectBase::Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val) {
  TRACE("CJSObjectBase::Wrap v8_isolate={} v8_val={}", P$(v8_isolate), v8_val);
  assert(!v8_val.IsEmpty());
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8_val->IsNull()) {
    return py::js_null();
  }
  if (v8_val->IsUndefined()) {
    return py::js_undefined();
  }
  if (v8_val->IsTrue()) {
    return py::bool_(true);
  }
  if (v8_val->IsFalse()) {
    return py::bool_(false);
  }
  if (v8_val->IsInt32()) {
    auto int32 = v8_val->Int32Value(v8_isolate->GetCurrentContext()).ToChecked();
    return py::int_(int32);
  }
  if (v8_val->IsString()) {
    auto v8_utf = v8u::toUTF(v8_isolate, v8_val.As<v8::String>());
    return py::str(*v8_utf, v8_utf.length());
  }
  if (v8_val->IsStringObject()) {
    auto v8_utf = v8u::toUTF(v8_isolate, v8_val.As<v8::StringObject>()->ValueOf());
    return py::str(*v8_utf, v8_utf.length());
  }
  if (v8_val->IsBoolean()) {
    bool val = v8_val->BooleanValue(v8_isolate);
    return py::bool_(val);
  }
  if (v8_val->IsBooleanObject()) {
    auto val = v8_val.As<v8::BooleanObject>()->BooleanValue(v8_isolate);
    return py::bool_(val);
  }
  if (v8_val->IsNumber()) {
    auto val = v8_val->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    return py::float_(val);
  }
  if (v8_val->IsNumberObject()) {
    auto val = v8_val.As<v8::NumberObject>()->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    return py::float_(val);
  }
  if (v8_val->IsDate()) {
    auto val = v8_val.As<v8::Date>()->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    auto ts = static_cast<time_t>(floor(val / 1000));
    auto t = localtime(&ts);
    auto u = (static_cast<int64_t>(floor(val))) % 1000 * 1000;
    return pythonFromDateAndTime(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, u);
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_obj = v8_val->ToObject(v8_context).ToLocalChecked();
  auto py_result = Wrap(v8_isolate, v8_obj);
  TRACE("CJSObjectBase::Wrap => {}", py_result);
  return py_result;
}

py::object CJSObjectBase::Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj) {
  TRACE("CJSObjectBase::Wrap v8_isolate={} v8_obj={}", P$(v8_isolate), v8_obj);
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withScope(v8_isolate);

  py::object py_result;
  auto traced_raw_object = lookupTracedObject(v8_obj);
  if (traced_raw_object) {
    py_result = py::reinterpret_borrow<py::object>(traced_raw_object);
  } else if (v8_obj.IsEmpty()) {
    // TODO: we should not treat empty values so softly, we should throw/crash
    py_result = py::none();
  } else {
    py_result = Wrap(v8_isolate, std::make_shared<CJSObject>(v8_obj));
  }

  TRACE("CJSObjectBase::Wrap => {}", py_result);
  return py_result;
}

py::object CJSObjectBase::Wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj) {
  TRACE("CJSObjectBase::Wrap v8_isolate={} obj={}", P$(v8_isolate), obj);
  auto py_gil = pyu::withGIL();

  if (v8u::executionTerminating(v8_isolate)) {
    return py::js_null();
  }

  auto py_result = py::cast(obj);
  TRACE("CJSObjectBase::Wrap => {}", py_result);
  return py_result;
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
