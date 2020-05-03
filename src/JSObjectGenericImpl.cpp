#include "JSObjectGenericImpl.h"
#include "JSException.h"
#include "Wrapping.h"
#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectGenericImplLogger), __VA_ARGS__)

static void ensureAttrExistsOrThrow(v8::IsolatePtr v8_isolate,
                                    v8::Local<v8::Object> v8_this,
                                    v8::Local<v8::String> v8_name) {
  TRACE("ensureAttrExistsOrThrow v8_name={} v8_this={}", v8_name, v8_this);
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8u::getCurrentContext(v8_isolate);

  auto hasName = v8_this->Has(v8_context, v8_name).FromMaybe(false);
  if (!hasName) {
    auto v8_proto_str = v8_this->ObjectProtoToString(v8_context).ToLocalChecked();
    auto v8_proto_utf = v8u::toUTF(v8_isolate, v8_proto_str);
    auto v8_name_utf = v8u::toUTF(v8_isolate, v8_name);
    auto msg = fmt::format("'{}' object has no attribute '{}'", *v8_proto_utf, *v8_name_utf);
    throw CJSException(msg, PyExc_AttributeError);
  }
}

py::object CJSObjectGenericImpl::GetAttr(const py::object& py_key) const {
  TRACE("CJSObjectGenericImpl::GetAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);

  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8u::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(v8_isolate, py_key);
  auto v8_this = m_base.ToV8(v8_isolate);
  ensureAttrExistsOrThrow(v8_isolate, v8_this, v8_attr_name);
  auto v8_attr_value = v8_this->Get(v8_context, v8_attr_name).ToLocalChecked();

  auto py_result = wrap(v8_isolate, v8_attr_value, v8_this);
  TRACE("CJSObjectGenericImpl::ObjectGetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObjectGenericImpl::SetAttr(const py::object& py_key, const py::object& py_obj) const {
  TRACE("CJSObjectGenericImpl::SetAttr {} name={} py_obj={}", THIS, py_key, py_obj);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);

  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8u::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(v8_isolate, py_key);
  auto v8_attr_obj = wrap(std::move(py_obj));

  auto v8_this = m_base.ToV8(v8_isolate);
  v8_this->Set(v8_context, v8_attr_name, v8_attr_obj).Check();
}

void CJSObjectGenericImpl::DelAttr(const py::object& py_key) const {
  TRACE("CJSObjectGenericImpl::DelAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);

  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8u::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(v8_isolate, py_key);
  auto v8_this = m_base.ToV8(v8_isolate);
  ensureAttrExistsOrThrow(v8_isolate, v8_this, v8_attr_name);

  v8_this->Delete(v8_context, v8_attr_name).Check();
}

bool CJSObjectGenericImpl::Contains(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);

  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8u::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  auto v8_this = m_base.ToV8(v8_isolate);
  bool result = v8_this->Has(v8_context, v8u::toString(v8_isolate, py_key)).ToChecked();
  TRACE("CJSObjectGenericImpl::Contains {} py_key={} => {}", THIS, py_key, result);
  return result;
}

py::str CJSObjectGenericImpl::Str() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);

  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_this = m_base.ToV8(v8_isolate);
  auto py_result = [&] {
    if (v8_this.IsEmpty()) {
      return py::str("<EMPTY>");
    } else {
      auto v8_context = v8u::getCurrentContext(v8_isolate);
      auto v8_str = v8_this->ToString(v8_context).ToLocalChecked();
      auto v8_utf = v8u::toUTF(v8_isolate, v8_str);
      return py::str(*v8_utf);
    }
  }();

  TRACE("CJSObjectGenericImpl::Str {} => {}", THIS, py_result);
  return py_result;
}

py::str CJSObjectGenericImpl::Repr() const {
  std::ostringstream ss;
  m_base.Dump(ss);
  auto str = fmt::format("JSObject[{}] {}", m_base.GetRoles(), ss.str());
  py::str py_result(str);
  TRACE("CJSObjectGenericImpl::Repr {} => {}", THIS, py_result);
  return py_result;
}
