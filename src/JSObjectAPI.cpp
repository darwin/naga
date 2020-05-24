#include "JSObject.h"
#include "JSObjectIterator.h"
#include "JSObjectArrayIterator.h"
#include "JSException.h"
#include "PythonThreads.h"
#include "Wrapping.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

py::object JSObjectAPI::GetAttr(const py::object& py_key) const {
  py::object py_result;
  if (HasRoleArray()) {
    throw JSException("__getattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else if (HasRoleArray()) {
    py_result = JSObjectCLJSGetAttr(Self(), py_key);
  } else {
    py_result = JSObjectGenericGetAttr(Self(), py_key);
  }
  TRACE("JSObjectAPI::GetAttr {} => {}", THIS, py_result);
  return py_result;
}

void JSObjectAPI::SetAttr(const py::object& py_key, const py::object& py_obj) const {
  if (HasRoleArray()) {
    throw JSException("__setattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    JSObjectGenericSetAttr(Self(), py_key, py_obj);
  }
}

void JSObjectAPI::DelAttr(const py::object& py_key) const {
  if (HasRoleArray()) {
    throw JSException("__delattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    JSObjectGenericDelAttr(Self(), py_key);
  }
}

py::list JSObjectAPI::Dir() const {
  TRACE("JSObjectAPI::Dir {}", THIS);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_this = ToV8(v8_isolate);
  auto v8_key_collection_mode = v8::KeyCollectionMode::kIncludePrototypes;
  auto v8_property_filter = static_cast<v8::PropertyFilter>(v8::PropertyFilter::ONLY_ENUMERABLE |  //
                                                            v8::PropertyFilter::SKIP_SYMBOLS);
  auto v8_index_filter = v8::IndexFilter::kSkipIndices;
  auto v8_conversion_mode = v8::KeyConversionMode::kNoNumbers;
  auto v8_maybe_property_names = v8_this->GetPropertyNames(v8_context,              //
                                                           v8_key_collection_mode,  //
                                                           v8_property_filter,      //
                                                           v8_index_filter,         //
                                                           v8_conversion_mode);
  auto v8_property_names = v8_maybe_property_names.ToLocalChecked();
  auto py_names = wrap(v8_isolate, v8_property_names);
  TRACE("JSObjectAPI::Dir {} => {}", THIS, py_names);
  return py_names;
}

py::object JSObjectAPI::GetItem(const py::object& py_key) const {
  py::object py_result;
  if (HasRoleArray()) {
    py_result = JSObjectArrayGetItem(Self(), py_key);
  } else if (HasRoleCLJS()) {
    py_result = JSObjectCLJSGetItem(Self(), py_key);
  } else {
    // TODO: do robust arg checking here
    py_result = JSObjectGenericGetAttr(Self(), py::cast<py::str>(py_key));
  }
  TRACE("JSObjectAPI::GetItem {} => {}", THIS, py_result);
  return py_result;
}

py::object JSObjectAPI::SetItem(const py::object& py_key, const py::object& py_value) const {
  if (HasRoleArray()) {
    return JSObjectArraySetItem(Self(), py_key, py_value);
  } else {
    // TODO: do robust arg checking here
    JSObjectGenericSetAttr(Self(), py::cast<py::str>(py_key), py_value);
    return py::none();
  }
}

py::object JSObjectAPI::DelItem(const py::object& py_key) const {
  if (HasRoleArray()) {
    return JSObjectArrayDelItem(Self(), py_key);
  } else {
    // TODO: do robust arg checking here
    JSObjectGenericDelAttr(Self(), py::cast<py::str>(py_key));
    return py::none();
  }
}

bool JSObjectAPI::Contains(const py::object& py_key) const {
  bool result;
  if (HasRoleArray()) {
    result = JSObjectArrayContains(Self(), py_key);
  } else {
    result = JSObjectGenericContains(Self(), py_key);
  }
  TRACE("JSObjectAPI::Contains {} => {}", THIS, result);
  return result;
}

int JSObjectAPI::Hash() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto result = ToV8(v8_isolate)->GetIdentityHash();
  TRACE("JSObjectAPI::Hash {} => {}", THIS, result);
  return result;
}

SharedJSObjectPtr JSObjectAPI::Clone() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto result = std::make_shared<JSObject>(ToV8(v8_isolate)->Clone());
  TRACE("JSObjectAPI::Clone {} => {}", THIS, result);
  return result;
}

bool JSObjectAPI::EQ(const SharedJSObjectPtr& other) const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  auto v8_context = v8x::getCurrentContext(v8_isolate);

  auto result = other.get() && ToV8(v8_isolate)->Equals(v8_context, other->ToV8(v8_isolate)).ToChecked();
  TRACE("JSObjectAPI::EQ {} other={} => {}", THIS, other, result);
  return result;
}

bool JSObjectAPI::NE(const SharedJSObjectPtr& other) const {
  return !EQ(other);
}

py::object JSObjectAPI::Int() const {
  TRACE("JSObjectAPI::ToPythonInt {}", THIS);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);

  if (m_v8_obj.IsEmpty()) {
    throw JSException("Argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = ToV8(v8_isolate)->Int32Value(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("JSObjectAPI::Int {} => {}", THIS, py_result);
  return py_result;
}

py::object JSObjectAPI::Float() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);

  if (m_v8_obj.IsEmpty()) {
    throw JSException("Argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = ToV8(v8_isolate)->NumberValue(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("JSObjectAPI::Float {} => {}", THIS, py_result);
  return py_result;
}

py::object JSObjectAPI::Bool() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);

  bool val = false;
  if (!m_v8_obj.IsEmpty()) {
    val = ToV8(v8_isolate)->BooleanValue(v8_isolate);
  }

  auto py_result = py::cast(val);
  TRACE("JSObjectAPI::Bool {} => {}", THIS, py_result);
  return py_result;
}

py::str JSObjectAPI::Str() const {
  auto py_result = [&] {
    if (HasRoleCLJS()) {
      return JSObjectCLJSStr(Self());
    } else {
      return JSObjectGenericStr(Self());
    }
  }();

  TRACE("JSObjectAPI::Str {} => {}", THIS, py_result);
  return py_result;
}

py::str JSObjectAPI::Repr() const {
  auto py_result = [&] {
    if (HasRoleCLJS()) {
      return JSObjectCLJSRepr(Self());
    } else {
      return JSObjectGenericRepr(Self());
    }
  }();

  TRACE("JSObjectAPI::Repr {} => {}", THIS, py_result);
  return py_result;
}

py::object JSObjectAPI::Iter() {
  TRACE("JSObjectAPI::Iter {}", THIS);
  if (HasRoleArray()) {
    return py::cast(std::make_shared<JSObjectArrayIterator>(static_cast<JSObject*>(this)->shared_from_this()));
  } else {
    return py::cast(std::make_shared<JSObjectIterator>(static_cast<JSObject*>(this)->shared_from_this()));
  }
}

size_t JSObjectAPI::Len() const {
  auto result = [&]() {
    if (HasRoleArray()) {
      return JSObjectArrayLength(Self());
    } else if (HasRoleCLJS()) {
      return JSObjectCLJSLength(Self());
    } else {
      return py::ssize_t{0};
    }
  }();

  TRACE("JSObjectAPI::Len {} => {}", THIS, result);
  return result;
}

py::object JSObjectAPI::Call(const py::args& py_args, const py::kwargs& py_kwargs) {
  py::object py_result;
  if (HasRoleFunction()) {
    py_result = JSObjectFunctionCall(Self(), py_args, py_kwargs);
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::Call {} => {}", THIS, py_result);
  return py_result;
}

py::object JSObjectAPI::Create(const SharedJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds) {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_proto = proto->ToV8(v8_isolate);
  if (v8_proto.IsEmpty()) {
    throw JSException("Object prototype may only be an Object", PyExc_TypeError);
  }

  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);

  if (!v8_proto->IsFunction()) {
    throw JSException("Object prototype expected to be a Function", PyExc_TypeError);
  }
  auto fn = v8_proto.As<v8::Function>();
  auto args_count = py_args.size();
  std::vector<v8::Local<v8::Value>> v8_args;
  v8_args.reserve(args_count);

  for (size_t i = 0; i < args_count; i++) {
    v8_args.push_back(wrap(py_args[i]));
  }

  auto v8_result = withAllowedPythonThreads(
      [&] { return fn->NewInstance(v8_context, v8_args.size(), v8_args.data()).ToLocalChecked(); });

  v8x::checkTryCatch(v8_isolate, v8_try_catch);

  auto it = py_kwds.begin();
  while (it != py_kwds.end()) {
    auto py_key = it->first;
    auto py_val = it->second;
    auto v8_key = v8x::toString(v8_isolate, py_key);
    auto v8_val = wrap(py_val);
    v8_result->Set(v8_context, v8_key, v8_val).Check();
    it++;
  }

  v8x::checkTryCatch(v8_isolate, v8_try_catch);

  return wrap(v8_isolate, v8_result);
}

py::object JSObjectAPI::Apply(const py::object& py_self, const py::list& py_args, const py::dict& py_kwds) {
  py::object py_result;
  if (HasRoleFunction()) {
    py_result = JSObjectFunctionApply(Self(), py_self, py_args, py_kwds);
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::Apply {} => {}", THIS, py_result);
  return py_result;
}

py::object JSObjectAPI::Invoke(const py::list& py_args, const py::dict& py_kwds) {
  py::object py_result;
  if (HasRoleFunction()) {
    py_result = JSObjectFunctionCall(Self(), py_args, py_kwds);
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::Invoke {} => {}", THIS, py_result);
  return py_result;
}

std::string JSObjectAPI::GetName() const {
  std::string result;
  if (HasRoleFunction()) {
    result = JSObjectFunctionGetName(Self());
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::GetName {} => {}", THIS, result);
  return result;
}

void JSObjectAPI::SetName(const std::string& name) {
  TRACE("JSObjectAPI::SetName {} name={}", THIS, name);
  if (HasRoleFunction()) {
    JSObjectFunctionSetName(Self(), name);
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }
}

int JSObjectAPI::LineNumber() const {
  int result;
  if (HasRoleFunction()) {
    result = JSObjectFunctionGetLineNumber(Self());
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::LineNumber {} => {}", THIS, result);
  return result;
}

int JSObjectAPI::ColumnNumber() const {
  int result;
  if (HasRoleFunction()) {
    result = JSObjectFunctionGetColumnNumber(Self());
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::ColumnNumber {} => {}", THIS, result);
  return result;
}

std::string JSObjectAPI::ResourceName() const {
  std::string result;
  if (HasRoleFunction()) {
    result = JSObjectFunctionGetResourceName(Self());
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::ResourceName {} => {}", THIS, result);
  return result;
}

std::string JSObjectAPI::InferredName() const {
  std::string result;
  if (HasRoleFunction()) {
    result = JSObjectFunctionGetInferredName(Self());
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::InferredName {} => {}", THIS, result);
  return result;
}

int JSObjectAPI::LineOffset() const {
  int result;
  if (HasRoleFunction()) {
    result = JSObjectFunctionGetLineOffset(Self());
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::LineOffset {} => {}", THIS, result);
  return result;
}

int JSObjectAPI::ColumnOffset() const {
  int result;
  if (HasRoleFunction()) {
    result = JSObjectFunctionGetColumnOffset(Self());
  } else {
    throw JSException("Expected JSObject with Function role", PyExc_TypeError);
  }

  TRACE("JSObjectAPI::ColumnOffset {} => {}", THIS, result);
  return result;
}

bool JSObjectAPI::HasRoleArray() const {
  return HasRole(JSObjectBase::Roles::Array);
}

bool JSObjectAPI::HasRoleFunction() const {
  return HasRole(JSObjectBase::Roles::Function);
}

bool JSObjectAPI::HasRoleCLJS() const {
  return HasRole(JSObjectBase::Roles::CLJS);
}
