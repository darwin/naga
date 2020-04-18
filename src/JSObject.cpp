#include "JSObject.h"
#include "JSObjectArray.h"
#include "JSObjectFunction.h"
#include "JSObjectCLJS.h"
#include "JSException.h"
#include "JSUndefined.h"
#include "JSNull.h"

#include "PythonObject.h"
#include "PythonDateTime.h"
#include "Tracer.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

void CJSObject::Expose(const py::module& py_module) {
  TRACE("CJSObject::Expose py_module={}", py_module);
  // clang-format off
  py::class_<CJSObject, CJSObjectPtr>(py_module, "JSObject")
      .def("__getattr__", &CJSObject::GetAttr)
      .def("__setattr__", &CJSObject::SetAttr)
      .def("__delattr__", &CJSObject::DelAttr)

      .def("__hash__", &CJSObject::GetIdentityHash)
      .def("clone", &CJSObject::Clone,
           "Clone the object.")
      .def("__dir__", &CJSObject::GetAttrList)

          // Emulating dict object
      .def("keys", &CJSObject::GetAttrList,
           "Get a list of the object attributes.")

      .def("__getitem__", &CJSObject::GetAttr)
      .def("__setitem__", &CJSObject::SetAttr)
      .def("__delitem__", &CJSObject::DelAttr)

      .def("__contains__", &CJSObject::Contains)

      .def("__int__", &CJSObject::ToPythonInt)
      .def("__float__", &CJSObject::ToPythonFloat)
      .def("__str__", &CJSObject::ToPythonStr)

      .def("__bool__", &CJSObject::ToPythonBool)
      .def("__eq__", &CJSObject::Equals)
      .def("__ne__", &CJSObject::Unequals)

      .def_static("create", &CJSObjectFunction::CreateWithArgs,
                  py::arg("constructor"),
                  py::arg("arguments") = py::tuple(),
                  py::arg("propertiesObject") = py::dict(),
                  "Creates a new object with the specified prototype object and properties.");

  py::class_<CJSObjectArray, CJSObjectArrayPtr, CJSObject>(py_module, "JSArray")
      .def(py::init<py::object>())

      .def("__len__", &CJSObjectArray::Length)

      .def("__getitem__", &CJSObjectArray::GetItem)
      .def("__setitem__", &CJSObjectArray::SetItem)
      .def("__delitem__", &CJSObjectArray::DelItem)

          // TODO:      .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)

      .def("__contains__", &CJSObjectArray::Contains);

  py::class_<CJSObjectFunction, CJSObjectFunctionPtr, CJSObject>(py_module, "JSFunction")
      .def("__call__", &CJSObjectFunction::CallWithArgs)

      .def("apply", &CJSObjectFunction::ApplyJavascript,
           py::arg("self"),
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a function call using the parameters.")
      .def("apply", &CJSObjectFunction::ApplyPython,
           py::arg("self"),
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a function call using the parameters.")
      .def("invoke", &CJSObjectFunction::Invoke,
           py::arg("args") = py::list(),
           py::arg("kwds") = py::dict(),
           "Performs a binding method call using the parameters.")

      .def("setName", &CJSObjectFunction::SetName)

      .def_property("name", &CJSObjectFunction::GetName, &CJSObjectFunction::SetName,
                    "The name of function")

      .def_property_readonly("linenum", &CJSObjectFunction::GetLineNumber,
                             "The line number of function in the script")
      .def_property_readonly("colnum", &CJSObjectFunction::GetColumnNumber,
                             "The column number of function in the script")
      .def_property_readonly("resname", &CJSObjectFunction::GetResourceName,
                             "The resource name of script")
      .def_property_readonly("inferredname", &CJSObjectFunction::GetInferredName,
                             "Name inferred from variable or property assignment of this function")
      .def_property_readonly("lineoff", &CJSObjectFunction::GetLineOffset,
                             "The line offset of function in the script")
      .def_property_readonly("coloff", &CJSObjectFunction::GetColumnOffset,
                             "The column offset of function in the script");
  // clang-format on
}

CJSObject::CJSObject(v8::Local<v8::Object> v8_obj) : m_v8_obj(v8u::getCurrentIsolate(), v8_obj) {
  TRACE("CJSObject::CJSObject {} v8_obj={}", THIS, v8_obj);
}

CJSObject::~CJSObject() {
  TRACE("CJSObject::~CJSObject {}", THIS);
  m_v8_obj.Reset();
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

py::object CJSObject::GetAttr(const std::string& name) {
  TRACE("CJSObject::GetAttr {} name={}", THIS, name);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  CheckAttr(v8_attr_name);

  auto v8_attr_value = Object()->Get(v8_context, v8_attr_name).ToLocalChecked();
  if (v8_attr_value.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  auto py_result = CJSObject::Wrap(v8_isolate, v8_attr_value, Object());
  TRACE("CJSObject::GetAttr {} => {}", THIS, py_result);
  return py_result;
}

void CJSObject::SetAttr(const std::string& name, py::object py_obj) const {
  TRACE("CJSObject::SetAttr {} name={} py_obj={}", THIS, name, py_obj);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  auto v8_attr_obj = CPythonObject::Wrap(std::move(py_obj));

  if (!Object()->Set(v8_context, v8_attr_name, v8_attr_obj).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

void CJSObject::DelAttr(const std::string& name) {
  TRACE("CJSObject::DelAttr {} name={}", THIS, name);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  CheckAttr(v8_attr_name);

  if (!Object()->Delete(v8_context, v8_attr_name).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

py::list CJSObject::GetAttrList() const {
  TRACE("CJSObject::GetAttrList {}", THIS);
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

  TRACE("CJSObject::GetAttrList {} => {}", THIS, attrs);
  return attrs;
}

int CJSObject::GetIdentityHash() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = Object()->GetIdentityHash();
  TRACE("CJSObject::GetIdentityHash {} => {}", THIS, result);
  return result;
}

CJSObjectPtr CJSObject::Clone() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  auto result = std::make_shared<CJSObject>(Object()->Clone());
  TRACE("CJSObject::Clone {} => {}", THIS, result);
  return result;
}

bool CJSObject::Contains(const std::string& name) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  v8::TryCatch try_catch(v8_isolate);

  bool result = Object()->Has(context, v8u::toString(name)).ToChecked();

  if (try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, try_catch);
  }

  TRACE("CJSObject::Contains {} name={} => {}", THIS, name, result);
  return result;
}

bool CJSObject::Equals(const CJSObjectPtr& other) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  v8::Local<v8::Context> context = v8_isolate->GetCurrentContext();

  auto result = other.get() && Object()->Equals(context, other->Object()).ToChecked();
  TRACE("CJSObject::Equals {} other={} => {}", THIS, other, result);
  return result;
}

py::object CJSObject::ToPythonInt() const {
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
  TRACE("CJSObject::ToPythonInt {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::ToPythonFloat() const {
  TRACE("CJSObject::ToPythonFloat {}", THIS);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->NumberValue(v8_context).ToChecked();
  auto py_result = py::cast(val);
  TRACE("CJSObject::ToPythonInt {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::ToPythonBool() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);

  bool val = false;
  if (!m_v8_obj.IsEmpty()) {
    val = Object()->BooleanValue(v8_isolate);
  }

  auto py_result = py::cast(val);
  TRACE("CJSObject::ToPythonBool {} => {}", THIS, py_result);
  return py_result;
}

py::object CJSObject::ToPythonStr() const {
  std::stringstream ss;
  ss << *this;
  auto py_result = py::cast(ss.str());
  TRACE("CJSObject::ToPythonStr {} => {}", THIS, py_result);
  return py_result;
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
      return Wrap(v8_isolate, std::make_shared<CJSObjectFunction>(v8_bound_fn));
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
    py_result = py::none();
  } else if (v8_obj->IsArray()) {
    auto v8_array = v8_obj.As<v8::Array>();
    py_result = Wrap(v8_isolate, std::make_shared<CJSObjectArray>(v8_array));
  }
#ifdef STPYV8_FEATURE_CLJS
  else if (isCLJSType(v8_obj)) {
    py_result = Wrap(v8_isolate, std::make_shared<CJSObjectCLJS>(v8_obj));
  }
#endif
  else if (v8_obj->IsFunction()) {
    // creating unbound function
    auto v8_fn = v8_obj.As<v8::Function>();
    py_result = Wrap(v8_isolate, std::make_shared<CJSObjectFunction>(v8_fn));
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
  TRACE("CJSObject::CJSObject {} => {}", THIS, v8_result);
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
