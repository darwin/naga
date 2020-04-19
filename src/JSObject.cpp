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

bool isCLJSType(v8::Local<v8::Object> v8_obj) {
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

void CJSObject::Expose(const py::module& py_module) {
  TRACE("CJSObject::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CJSObject, CJSObjectPtr>(py_module, "JSObject")
      .def("__getattr__", &CJSObject::PythonGetAttr)
      .def("__setattr__", &CJSObject::PythonSetAttr)
      .def("__delattr__", &CJSObject::PythonDelAttr)

      .def("__hash__", &CJSObject::PythonIdentityHash)
      // TODO: I'm not sure about this, revisit
      .def("clone", &CJSObject::PythonClone,
           "Clone the object.")
      .def("__dir__", &CJSObject::PythonGetAttrList)

      // Emulating dict object
      // TODO: I'm not sure about this, revisit
      .def("keys", &CJSObject::PythonGetAttrList,
           "Get a list of the object attributes.")

      .def("__getitem__", &CJSObject::PythonGetItem)
      .def("__setitem__", &CJSObject::PythonSetItem)
      .def("__delitem__", &CJSObject::PythonDelItem)

      .def("__contains__", &CJSObject::PythonContains)
      .def("__len__", &CJSObject::PythonLength)

      .def("__int__", &CJSObject::PythonInt)
      .def("__float__", &CJSObject::PythonFloat)
      .def("__str__", &CJSObject::PythonStr)
      .def("__repr__", &CJSObject::PythonRepr)
      .def("__bool__", &CJSObject::PythonBool)

      .def("__eq__", &CJSObject::PythonEquals)
      .def("__ne__", &CJSObject::PythonNotEquals)

      .def_static("create", &CJSObject::PythonCreateWithArgs,
                  py::arg("constructor"),
                  py::arg("arguments") = py::tuple(),
                  py::arg("propertiesObject") = py::dict(),
                  "Creates a new object with the specified prototype object and properties.")

      .def_static("hasJSArrayRole", [](const CJSObjectPtr &obj) {
          return obj->HasRole(Roles::JSArray);
      })
      .def_static("hasJSFunctionRole", [](const CJSObjectPtr &obj) {
          return obj->HasRole(Roles::JSFunction);
      })
      .def_static("hasCLJSObjectRole", [](const CJSObjectPtr &obj) {
          return obj->HasRole(Roles::CLJSObject);
      })

          // JSFunction
      .def("__call__", &CJSObject::PythonCallWithArgs)

      .def("apply", &CJSObject::ApplyJavascript,
           py::arg("self"),
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a function call using the parameters.")
      .def("apply", &CJSObject::ApplyPython,
           py::arg("self"),
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a function call using the parameters.")
      .def("invoke", &CJSObject::Invoke,
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a binding method call using the parameters.")

// TODO: revisit this, there is a clash with normal attribute lookups
//      .def("setName", &CJSObject::SetName)
//
//      .def_property("name", &CJSObject::GetName, &CJSObject::SetName,
//                    "The name of function")

      .def_property_readonly("linenum", &CJSObject::GetLineNumber,
                             "The line number of function in the script")
      .def_property_readonly("colnum", &CJSObject::GetColumnNumber,
                             "The column number of function in the script")
      .def_property_readonly("resname", &CJSObject::GetResourceName,
                             "The resource name of script")
      .def_property_readonly("inferredname", &CJSObject::GetInferredName,
                             "Name inferred from variable or property assignment of this function")
      .def_property_readonly("lineoff", &CJSObject::GetLineOffset,
                             "The line offset of function in the script")
      .def_property_readonly("coloff", &CJSObject::GetColumnOffset,
                             "The column offset of function in the script")

    // TODO:      .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)
      ;
  // clang-format on
}

CJSObject::CJSObject(v8::Local<v8::Object> v8_obj)
    : m_roles(Roles::JSObject), m_v8_obj(v8u::getCurrentIsolate(), v8_obj) {
  // detect supported object roles
  if (v8_obj->IsFunction()) {
    m_roles |= Roles::JSFunction;
  }
  if (v8_obj->IsArray()) {
    m_roles |= Roles::JSArray;
  }
#ifdef STPYV8_FEATURE_CLJS
  if (isCLJSType(v8_obj)) {
    m_roles |= Roles::CLJSObject;
  }
#endif
  TRACE("CJSObject::CJSObject {} v8_obj={} roles={}", THIS, v8_obj, roles_printer{m_roles});
}

CJSObject::~CJSObject() {
  TRACE("CJSObject::~CJSObject {}", THIS);
  m_v8_obj.Reset();
}

bool CJSObject::HasRole(Roles roles) const {
  return (m_roles & roles) == roles;
}

bool CJSObject::PythonContains(const py::object& py_key) const {
  bool result;
  if (HasRole(Roles::JSArray)) {
    result = ArrayContains(py_key);
  } else {
    result = ObjectContains(py_key);
  }
  TRACE("CJSObject::PythonContains {} => {}", THIS, result);
  return result;
}

py::object CJSObject::PythonGetItem(py::object py_key) const {
  py::object py_result;
  if (HasRole(Roles::JSArray)) {
    py_result = ArrayGetItem(py_key);
  } else if (HasRole(Roles::CLJSObject)) {
    py_result = CLJSGetItem(py_key);
  } else {
    // TODO: do robust arg checking here
    py_result = ObjectGetAttr(py::cast<py::str>(py_key));
  }
  TRACE("CJSObject::PythonGetItem {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::PythonSetItem(py::object py_key, py::object py_value) const {
  if (HasRole(Roles::JSArray)) {
    return ArraySetItem(py_key, py_value);
  } else {
    // TODO: do robust arg checking here
    ObjectSetAttr(py::cast<py::str>(py_key), py_value);
    return py::none();
  }
}

py::object CJSObject::PythonDelItem(py::object py_key) const {
  if (HasRole(Roles::JSArray)) {
    return ArrayDelItem(py_key);
  } else {
    // TODO: do robust arg checking here
    ObjectDelAttr(py::cast<py::str>(py_key));
    return py::none();
  }
}

void CJSObject::CheckAttr(v8::Local<v8::String> v8_name) const {
  TRACE("CJSObject::CheckAttr {} v8_name={}", THIS, v8_name);
  auto v8_isolate = v8u::getCurrentIsolate();
  assert(v8_isolate->InContext());

  auto v8_scope = v8u::withScope(v8_isolate);
  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  if (!Object()->Has(context, v8_name).FromMaybe(false)) {
    std::ostringstream msg;

    msg << "'" << *v8::String::Utf8Value(v8_isolate, Object()->ObjectProtoToString(context).ToLocalChecked())
        << "' object has no attribute '" << *v8::String::Utf8Value(v8_isolate, v8_name) << "'";

    throw CJSException(msg.str(), PyExc_AttributeError);
  }
}

py::object CJSObject::PythonGetAttr(py::object py_key) const {
  py::object py_result;
  if (HasRole(Roles::JSArray)) {
    throw CJSException("__getattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else if (HasRole(Roles::JSArray)) {
    py_result = CLJSGetAttr(py_key);
  } else {
    py_result = ObjectGetAttr(py_key);
  }
  TRACE("CJSObject::PythonGetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObject::PythonSetAttr(py::object py_key, py::object py_obj) const {
  if (HasRole(Roles::JSArray)) {
    throw CJSException("__setattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    ObjectSetAttr(py_key, py_obj);
  }
}

void CJSObject::PythonDelAttr(py::object py_key) const {
  if (HasRole(Roles::JSArray)) {
    throw CJSException("__delattr__ not implemented for JSObjects with Array role", PyExc_AttributeError);
  } else {
    ObjectDelAttr(py_key);
  }
}

py::object CJSObject::ObjectGetAttr(py::object py_key) const {
  TRACE("CJSObject::ObjectGetAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);

  auto v8_attr_value = Object()->Get(v8_context, v8_attr_name).ToLocalChecked();
  if (v8_attr_value.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  auto py_result = CJSObject::Wrap(v8_isolate, v8_attr_value, Object());
  TRACE("CJSObject::ObjectGetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObject::ObjectSetAttr(py::object py_key, py::object py_obj) const {
  TRACE("CJSObject::ObjectSetAttr {} name={} py_obj={}", THIS, py_key, py_obj);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  auto v8_attr_obj = CPythonObject::Wrap(std::move(py_obj));

  if (!Object()->Set(v8_context, v8_attr_name, v8_attr_obj).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

void CJSObject::ObjectDelAttr(py::object py_key) const {
  TRACE("CJSObject::ObjectDelAttr {} name={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(py_key);
  CheckAttr(v8_attr_name);

  if (!Object()->Delete(v8_context, v8_attr_name).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

py::list CJSObject::PythonGetAttrList() const {
  TRACE("CJSObject::PythonGetAttrList {}", THIS);
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
    attrs.append(CJSObject::Wrap(v8_isolate, v8_prop));
  }

  if (v8_try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  TRACE("CJSObject::PythonGetAttrList {} => {}", THIS, attrs);
  return attrs;
}

int CJSObject::PythonIdentityHash() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = Object()->GetIdentityHash();
  TRACE("CJSObject::PythonIdentityHash {} => {}", THIS, result);
  return result;
}

CJSObjectPtr CJSObject::PythonClone() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = std::make_shared<CJSObject>(Object()->Clone());
  TRACE("CJSObject::PythonClone {} => {}", THIS, result);
  return result;
}

bool CJSObject::ObjectContains(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  v8::TryCatch try_catch(v8_isolate);

  bool result = Object()->Has(context, v8u::toString(py_key)).ToChecked();

  if (try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, try_catch);
  }

  TRACE("CJSObject::ObjectContains {} py_key={} => {}", THIS, py_key, result);
  return result;
}

bool CJSObject::PythonEquals(const CJSObjectPtr& other) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  auto result = other.get() && Object()->Equals(context, other->Object()).ToChecked();
  TRACE("CJSObject::PythonEquals {} other={} => {}", THIS, other, result);
  return result;
}

bool CJSObject::PythonNotEquals(const CJSObjectPtr& other) const {
  return !PythonEquals(other);
}

py::object CJSObject::PythonInt() const {
  TRACE("CJSObject::ToPythonInt {}", THIS);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->Int32Value(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("CJSObject::PythonInt {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::PythonFloat() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->NumberValue(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("CJSObject::PythonFloat {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::PythonBool() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  bool val = false;
  if (!m_v8_obj.IsEmpty()) {
    val = Object()->BooleanValue(v8_isolate);
  }

  auto py_result = py::cast(val);
  TRACE("CJSObject::PythonBool {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::PythonStr() const {
  py::object py_result;
  if (HasRole(Roles::CLJSObject)) {
    py_result = CLJSStr();
  } else {
    std::stringstream ss;
    ss << *this;
    py_result = py::cast(ss.str());
  }

  TRACE("CJSObject::PythonStr {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::PythonRepr() const {
  py::object py_result;
  if (HasRole(Roles::CLJSObject)) {
    py_result = CLJSRepr();
  } else {
    auto s = fmt::format("JSObject{}", roles_printer{m_roles});
    py_result = py::cast(s);
  }

  TRACE("CJSObject::PythonRepr {} => {}", THIS, py_result);
  return py_result;
}

size_t CJSObject::PythonLength() const {
  size_t result;
  if (HasRole(Roles::JSArray)) {
    result = ArrayLength();
  } else if (HasRole(Roles::CLJSObject)) {
    result = CLJSLength();
  } else {
    result = 0;
  }

  TRACE("CJSObject::PythonLength {} => {}", THIS, result);
  return result;
}

py::object CJSObject::Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this) {
  TRACE("CJSObject::Wrap v8_isolate={} v8_val={} v8_this={}", isolateref_printer{v8_isolate}, v8_val, v8_this);

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
      TRACE("CJSObject::Wrap v8_bound_fn={}", v8_bound_fn);
      // TODO: mark as function role?
      return Wrap(v8_isolate, std::make_shared<CJSObject>(v8_bound_fn));
    }
  }

  return Wrap(v8_isolate, v8_val);
}

py::object CJSObject::Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val) {
  TRACE("CJSObject::Wrap v8_isolate={} v8_val={}", isolateref_printer{v8_isolate}, v8_val);
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
  TRACE("CJSObject::Wrap => {}", py_result);
  return py_result;
}

py::object CJSObject::Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj) {
  TRACE("CJSObject::Wrap v8_isolate={} v8_obj={}", isolateref_printer{v8_isolate}, v8_obj);
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

  TRACE("CJSObject::Wrap => {}", py_result);
  return py_result;
}

py::object CJSObject::Wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj) {
  TRACE("CJSObject::Wrap v8_isolate={} obj={}", isolateref_printer{v8_isolate}, obj);
  auto py_gil = pyu::withGIL();

  if (v8u::executionTerminating(v8_isolate)) {
    return py::js_null();
  }

  auto py_result = py::cast(obj);
  TRACE("CJSObject::Wrap => {}", py_result);
  return py_result;
}

v8::Local<v8::Object> CJSObject::Object() const {
  // TODO: isolate should be taken from the m_v8_obj
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_result = v8::Local<v8::Object>::New(v8_isolate, m_v8_obj);
  TRACE("CJSObject::Object {} => {}", THIS, v8_result);
  return v8_result;
}

void CJSObject::Dump(std::ostream& os) const {
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
