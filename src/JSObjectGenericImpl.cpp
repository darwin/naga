#include "JSObjectGenericImpl.h"
#include "JSException.h"
#include "PythonObject.h"
#include "Wrapping.h"
#include "JSObject.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectGenericImplLogger), __VA_ARGS__)

void CJSObjectGenericImpl::CheckAttr(v8::Local<v8::String> v8_name) const {
  TRACE("CJSObjectGenericImpl::CheckAttr {} v8_name={}", THIS, v8_name);
  auto v8_isolate = v8u::getCurrentIsolate();
  assert(v8_isolate->InContext());

  auto v8_scope = v8u::withScope(v8_isolate);
  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  if (!m_base.ToV8(v8_isolate)->Has(context, v8_name).FromMaybe(false)) {
    std::ostringstream msg;

    msg << "'"
        << *v8::String::Utf8Value(v8_isolate, m_base.ToV8(v8_isolate)->ObjectProtoToString(context).ToLocalChecked())
        << "' object has no attribute '" << *v8::String::Utf8Value(v8_isolate, v8_name) << "'";

    throw CJSException(msg.str(), PyExc_AttributeError);
  }
}

py::object CJSObjectGenericImpl::GetAttr(const py::object& py_key) const {
  TRACE("CJSObjectGenericImpl::GetAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);
  auto v8_attr_value = m_base.ToV8(v8_isolate)->Get(v8_context, v8_attr_name).ToLocalChecked();

  auto py_result = wrap(v8_isolate, v8_attr_value, m_base.ToV8(v8_isolate));
  TRACE("CJSObjectGenericImpl::ObjectGetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObjectGenericImpl::SetAttr(const py::object& py_key, const py::object& py_obj) const {
  TRACE("CJSObjectGenericImpl::SetAttr {} name={} py_obj={}", THIS, py_key, py_obj);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  auto v8_attr_obj = wrap(std::move(py_obj));

  m_base.ToV8(v8_isolate)->Set(v8_context, v8_attr_name, v8_attr_obj).Check();
}

void CJSObjectGenericImpl::DelAttr(const py::object& py_key) const {
  TRACE("CJSObjectGenericImpl::DelAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);

  m_base.ToV8(v8_isolate)->Delete(v8_context, v8_attr_name).Check();
}

bool CJSObjectGenericImpl::Contains(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  bool result = m_base.ToV8(v8_isolate)->Has(v8_context, v8u::toString(py_key)).ToChecked();
  TRACE("CJSObjectGenericImpl::Contains {} py_key={} => {}", THIS, py_key, result);
  return result;
}

py::str CJSObjectGenericImpl::Str() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_obj = m_base.ToV8(v8_isolate);
  auto py_result = [&] {
    if (v8_obj.IsEmpty()) {
      return py::str("<EMPTY>");
    } else {
      auto v8_context = v8_isolate->GetCurrentContext();
      auto v8_str = v8_obj->ToString(v8_context).ToLocalChecked();
      auto v8_utf = v8u::toUTF(v8_isolate, v8_str);
      return py::str(*v8_utf);
    }
  }();

  TRACE("CJSObjectGenericImpl::Str {} => {}", THIS, py_result);
  return py_result;
}

py::str CJSObjectGenericImpl::Repr() const {
  std::stringstream ss;
  m_base.Dump(ss);
  auto s = fmt::format("JSObject[{}] {}", m_base.GetRoles(), ss.str());
  py::str py_result(s);
  TRACE("CJSObjectGenericImpl::Repr {} => {}", THIS, py_result);
  return py_result;
}
