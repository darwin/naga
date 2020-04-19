#include "JSObject.h"
#include "JSException.h"
#include "PythonAllowThreadsGuard.h"
#include "PythonObject.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

py::object CJSObjectAPI::PythonGetAttr(const py::object& py_key) const {
  py::object py_result;
  if (HasRole(Roles::JSArray)) {
    throw CJSException("__getattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else if (HasRole(Roles::JSArray)) {
    py_result = m_cljs_impl.CLJSGetAttr(py_key);
  } else {
    py_result = m_generic_impl.ObjectGetAttr(py_key);
  }
  TRACE("CJSObjectAPI::PythonGetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObjectAPI::PythonSetAttr(const py::object& py_key, const py::object& py_obj) const {
  if (HasRole(Roles::JSArray)) {
    throw CJSException("__setattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    m_generic_impl.ObjectSetAttr(py_key, py_obj);
  }
}

void CJSObjectAPI::PythonDelAttr(const py::object& py_key) const {
  if (HasRole(Roles::JSArray)) {
    throw CJSException("__delattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    m_generic_impl.ObjectDelAttr(py_key);
  }
}

py::list CJSObjectAPI::PythonGetAttrList() const {
  TRACE("CJSObjectAPI::PythonGetAttrList {}", THIS);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  auto py_gil = pyu::withGIL();

  py::list attrs;

  if (v8u::executionTerminating(v8_isolate)) {
    return attrs;
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto props = Object()->GetPropertyNames(v8_context).ToLocalChecked();

  for (size_t i = 0; i < props->Length(); i++) {
    auto v8_i = v8::Integer::New(v8_isolate, i);
    auto v8_prop = props->Get(v8_context, v8_i).ToLocalChecked();
    attrs.append(CJSObjectAPI::Wrap(v8_isolate, v8_prop));
  }

  if (v8_try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  TRACE("CJSObjectAPI::PythonGetAttrList {} => {}", THIS, attrs);
  return attrs;
}

py::object CJSObjectAPI::PythonGetItem(const py::object& py_key) const {
  py::object py_result;
  if (HasRole(Roles::JSArray)) {
    py_result = m_array_impl.ArrayGetItem(py_key);
  } else if (HasRole(Roles::CLJSObject)) {
    py_result = m_cljs_impl.CLJSGetItem(py_key);
  } else {
    // TODO: do robust arg checking here
    py_result = m_generic_impl.ObjectGetAttr(py::cast<py::str>(py_key));
  }
  TRACE("CJSObjectAPI::PythonGetItem {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::PythonSetItem(const py::object& py_key, const py::object& py_value) const {
  if (HasRole(Roles::JSArray)) {
    return m_array_impl.ArraySetItem(py_key, py_value);
  } else {
    // TODO: do robust arg checking here
    m_generic_impl.ObjectSetAttr(py::cast<py::str>(py_key), py_value);
    return py::none();
  }
}

py::object CJSObjectAPI::PythonDelItem(const py::object& py_key) const {
  if (HasRole(Roles::JSArray)) {
    return m_array_impl.ArrayDelItem(py_key);
  } else {
    // TODO: do robust arg checking here
    m_generic_impl.ObjectDelAttr(py::cast<py::str>(py_key));
    return py::none();
  }
}

bool CJSObjectAPI::PythonContains(const py::object& py_key) const {
  bool result;
  if (HasRole(Roles::JSArray)) {
    result = m_array_impl.ArrayContains(py_key);
  } else {
    result = m_generic_impl.ObjectContains(py_key);
  }
  TRACE("CJSObjectAPI::PythonContains {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::PythonIdentityHash() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = Object()->GetIdentityHash();
  TRACE("CJSObjectAPI::PythonIdentityHash {} => {}", THIS, result);
  return result;
}

CJSObjectPtr CJSObjectAPI::PythonClone() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = std::make_shared<CJSObject>(Object()->Clone());
  TRACE("CJSObjectAPI::PythonClone {} => {}", THIS, result);
  return result;
}

bool CJSObjectAPI::PythonEquals(const CJSObjectPtr& other) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  auto result = other.get() && Object()->Equals(context, other->Object()).ToChecked();
  TRACE("CJSObjectAPI::PythonEquals {} other={} => {}", THIS, other, result);
  return result;
}

bool CJSObjectAPI::PythonNotEquals(const CJSObjectPtr& other) const {
  return !PythonEquals(other);
}

py::object CJSObjectAPI::PythonInt() const {
  TRACE("CJSObjectAPI::ToPythonInt {}", THIS);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->Int32Value(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("CJSObjectAPI::PythonInt {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::PythonFloat() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->NumberValue(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("CJSObjectAPI::PythonFloat {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::PythonBool() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  bool val = false;
  if (!m_v8_obj.IsEmpty()) {
    val = Object()->BooleanValue(v8_isolate);
  }

  auto py_result = py::cast(val);
  TRACE("CJSObjectAPI::PythonBool {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::PythonStr() const {
  py::object py_result;
  if (HasRole(Roles::CLJSObject)) {
    py_result = m_cljs_impl.CLJSStr();
  } else {
    std::stringstream ss;
    ss << *this;
    py_result = py::cast(ss.str());
  }

  TRACE("CJSObjectAPI::PythonStr {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::PythonRepr() const {
  py::object py_result;
  if (HasRole(Roles::CLJSObject)) {
    py_result = m_cljs_impl.CLJSRepr();
  } else {
    auto s = fmt::format("JSObject{}", roles_printer{m_roles});
    py_result = py::cast(s);
  }

  TRACE("CJSObjectAPI::PythonRepr {} => {}", THIS, py_result);
  return py_result;
}

size_t CJSObjectAPI::PythonLength() const {
  size_t result;
  if (HasRole(Roles::JSArray)) {
    result = m_array_impl.ArrayLength();
  } else if (HasRole(Roles::CLJSObject)) {
    result = m_cljs_impl.CLJSLength();
  } else {
    result = 0;
  }

  TRACE("CJSObjectAPI::PythonLength {} => {}", THIS, result);
  return result;
}

py::object CJSObjectAPI::PythonCallWithArgs(const py::args& py_args, const py::kwargs& py_kwargs) {
  py::object py_result;
  if (HasRole(Roles::JSFunction)) {
    py_result = m_function_impl.Call(py_args, py_kwargs);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonCallWithArgs {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::PythonCreateWithArgs(const CJSObjectPtr& proto,
                                              const py::tuple& py_args,
                                              const py::dict& py_kwds) {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  if (proto->Object().IsEmpty()) {
    throw CJSException("Object prototype may only be an Object", PyExc_TypeError);
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto fn = proto->Object().As<v8::Function>();
  auto args_count = py_args.size();
  std::vector<v8::Local<v8::Value>> v8_params(args_count);

  for (size_t i = 0; i < args_count; i++) {
    v8_params[i] = CPythonObject::Wrap(py_args[i]);
  }

  v8::Local<v8::Object> v8_result;

  withPythonAllowThreadsGuard(
      [&]() { v8_result = fn->NewInstance(v8_context, v8_params.size(), v8_params.data()).ToLocalChecked(); });

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  auto it = py_kwds.begin();
  while (it != py_kwds.end()) {
    auto py_key = it->first;
    auto py_val = it->second;
    auto v8_key = v8u::toString(py_key);
    auto v8_val = CPythonObject::Wrap(py_val);
    v8_result->Set(v8_context, v8_key, v8_val).Check();
    it++;
  }

  return Wrap(v8_isolate, v8_result);
}

py::object CJSObjectAPI::PythonApply(const py::object& py_self, const py::list& py_args, const py::dict& py_kwds) {
  py::object py_result;
  if (HasRole(Roles::JSFunction)) {
    py_result = m_function_impl.Apply(py_self, py_args, py_kwds);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonApply {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObjectAPI::PythonInvoke(const py::list& py_args, const py::dict& py_kwds) {
  py::object py_result;
  if (HasRole(Roles::JSFunction)) {
    py_result = m_function_impl.Call(py_args, py_kwds);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonInvoke {} => {}", THIS, py_result);
  return py_result;
}

std::string CJSObjectAPI::PythonGetName() const {
  std::string result;
  if (HasRole(Roles::JSFunction)) {
    result = m_function_impl.GetName();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonGetName {} => {}", THIS, result);
  return result;
}

void CJSObjectAPI::PythonSetName(const std::string& name) {
  TRACE("CJSObjectAPI::PythonSetName {} name={}", THIS, name);
  if (HasRole(Roles::JSFunction)) {
    m_function_impl.SetName(name);
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }
}

int CJSObjectAPI::PythonLineNumber() const {
  int result;
  if (HasRole(Roles::JSFunction)) {
    result = m_function_impl.GetLineNumber();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonLineNumber {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::PythonColumnNumber() const {
  int result;
  if (HasRole(Roles::JSFunction)) {
    result = m_function_impl.GetColumnNumber();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonColumnNumber {} => {}", THIS, result);
  return result;
}

std::string CJSObjectAPI::PythonResourceName() const {
  std::string result;
  if (HasRole(Roles::JSFunction)) {
    result = m_function_impl.GetResourceName();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonResourceName {} => {}", THIS, result);
  return result;
}

std::string CJSObjectAPI::PythonInferredName() const {
  std::string result;
  if (HasRole(Roles::JSFunction)) {
    result = m_function_impl.GetInferredName();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonInferredName {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::PythonLineOffset() const {
  int result;
  if (HasRole(Roles::JSFunction)) {
    result = m_function_impl.GetLineOffset();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonLineOffset {} => {}", THIS, result);
  return result;
}

int CJSObjectAPI::PythonColumnOffset() const {
  int result;
  if (HasRole(Roles::JSFunction)) {
    result = m_function_impl.GetColumnOffset();
  } else {
    throw CJSException("expect JSObject with Function role", PyExc_TypeError);
  }

  TRACE("CJSObjectAPI::PythonColumnOffset {} => {}", THIS, result);
  return result;
}