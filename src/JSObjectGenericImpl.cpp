#include "JSObject.h"
#include "JSException.h"
#include "JSUndefined.h"
#include "JSNull.h"

#include "PythonObject.h"
#include "PythonDateTime.h"
#include "Tracer.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

void CJSObjectGenericImpl::CheckAttr(v8::Local<v8::String> v8_name) const {
  TRACE("CJSObjectGenericImpl::CheckAttr {} v8_name={}", THIS, v8_name);
  auto v8_isolate = v8u::getCurrentIsolate();
  assert(v8_isolate->InContext());

  auto v8_scope = v8u::withScope(v8_isolate);
  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  if (!m_base->Object()->Has(context, v8_name).FromMaybe(false)) {
    std::ostringstream msg;

    msg << "'" << *v8::String::Utf8Value(v8_isolate, m_base->Object()->ObjectProtoToString(context).ToLocalChecked())
        << "' object has no attribute '" << *v8::String::Utf8Value(v8_isolate, v8_name) << "'";

    throw CJSException(msg.str(), PyExc_AttributeError);
  }
}

py::object CJSObjectGenericImpl::ObjectGetAttr(py::object py_key) const {
  TRACE("CJSObjectGenericImpl::ObjectGetAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);

  auto v8_attr_value = m_base->Object()->Get(v8_context, v8_attr_name).ToLocalChecked();
  if (v8_attr_value.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  auto py_result = CJSObject::Wrap(v8_isolate, v8_attr_value, m_base->Object());
  TRACE("CJSObjectGenericImpl::ObjectGetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObjectGenericImpl::ObjectSetAttr(py::object py_key, py::object py_obj) const {
  TRACE("CJSObjectGenericImpl::ObjectSetAttr {} name={} py_obj={}", THIS, py_key, py_obj);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  auto v8_attr_obj = CPythonObject::Wrap(std::move(py_obj));

  if (!m_base->Object()->Set(v8_context, v8_attr_name, v8_attr_obj).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

void CJSObjectGenericImpl::ObjectDelAttr(py::object py_key) const {
  TRACE("CJSObjectGenericImpl::ObjectDelAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);

  if (!m_base->Object()->Delete(v8_context, v8_attr_name).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

bool CJSObjectGenericImpl::ObjectContains(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  v8::TryCatch try_catch(v8_isolate);

  bool result = m_base->Object()->Has(context, v8u::toString(py_key)).ToChecked();

  if (try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, try_catch);
  }

  TRACE("CJSObjectGenericImpl::ObjectContains {} py_key={} => {}", THIS, py_key, result);
  return result;
}