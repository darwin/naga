#include "JSObjectGenericImpl.h"
#include "JSException.h"
#include "PythonObject.h"
#include "Wrapping.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectGenericImplLogger), __VA_ARGS__)

void CJSObjectGenericImpl::CheckAttr(v8::Local<v8::String> v8_name) const {
  TRACE("CJSObjectGenericImpl::CheckAttr {} v8_name={}", THIS, v8_name);
  auto v8_isolate = v8u::getCurrentIsolate();
  assert(v8_isolate->InContext());

  auto v8_scope = v8u::withScope(v8_isolate);
  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  if (!m_base.Object()->Has(context, v8_name).FromMaybe(false)) {
    std::ostringstream msg;

    msg << "'" << *v8::String::Utf8Value(v8_isolate, m_base.Object()->ObjectProtoToString(context).ToLocalChecked())
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
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);

  auto v8_attr_value = m_base.Object()->Get(v8_context, v8_attr_name).ToLocalChecked();
  CJSException::HandleTryCatch(v8_isolate, v8_try_catch);

  auto py_result = wrap(v8_isolate, v8_attr_value, m_base.Object());
  TRACE("CJSObjectGenericImpl::ObjectGetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObjectGenericImpl::SetAttr(const py::object& py_key, const py::object& py_obj) const {
  TRACE("CJSObjectGenericImpl::SetAttr {} name={} py_obj={}", THIS, py_key, py_obj);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  auto v8_attr_obj = wrap(std::move(py_obj));

  if (!m_base.Object()->Set(v8_context, v8_attr_name, v8_attr_obj).FromMaybe(false)) {
    CJSException::HandleTryCatch(v8_isolate, v8_try_catch);
  }
}

void CJSObjectGenericImpl::DelAttr(const py::object& py_key) const {
  TRACE("CJSObjectGenericImpl::DelAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);

  if (!m_base.Object()->Delete(v8_context, v8_attr_name).FromMaybe(false)) {
    CJSException::HandleTryCatch(v8_isolate, v8_try_catch);
  }
}

bool CJSObjectGenericImpl::Contains(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  v8::TryCatch try_catch(v8_isolate);

  bool result = m_base.Object()->Has(context, v8u::toString(py_key)).ToChecked();

  CJSException::HandleTryCatch(v8_isolate, try_catch);

  TRACE("CJSObjectGenericImpl::Contains {} py_key={} => {}", THIS, py_key, result);
  return result;
}