#include "JSObject.h"
#include "JSObjectNull.h"
#include "JSObjectUndefined.h"
#include "JSObjectArray.h"
#include "JSObjectFunction.h"
#include "JSObjectCLJS.h"
#include "JSException.h"

#include "PythonObject.h"
#include "PythonDateTime.h"
#include "PythonGIL.h"

static std::ostream& operator<<(std::ostream& os, const CJSObject& obj) {
  obj.Dump(os);
  return os;
}

void CJSObject::Expose(const py::module& py_module) {
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

  py::class_<CJSObjectNull, CJSObjectNullPtr, CJSObject>(py_module, "JSNull")
      .def(py::init<>())
      .def("__bool__", &CJSObjectNull::nonzero)
      .def("__str__", &CJSObjectNull::str);

  py::class_<CJSObjectUndefined, CJSObjectUndefinedPtr, CJSObject>(py_module, "JSUndefined")
      .def(py::init<>())
      .def("__bool__", &CJSObjectUndefined::nonzero)
      .def("__str__", &CJSObjectUndefined::str);

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
      .def_property_readonly("owner", &CJSObjectFunction::GetOwner2)

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

  // exposeCLJSTypes();
}

void CJSObject::CheckAttr(v8::Local<v8::String> v8_name) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  assert(isolate->InContext());

  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (!Object()->Has(context, v8_name).FromMaybe(false)) {
    std::ostringstream msg;

    msg << "'" << *v8::String::Utf8Value(isolate, Object()->ObjectProtoToString(context).ToLocalChecked())
        << "' object has no attribute '" << *v8::String::Utf8Value(isolate, v8_name) << "'";

    throw CJSException(msg.str(), PyExc_AttributeError);
  }
}

py::object CJSObject::GetAttr(const std::string& name) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  CheckAttr(v8_attr_name);

  auto v8_attr_value = Object()->Get(v8_context, v8_attr_name).ToLocalChecked();
  if (v8_attr_value.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return CJSObject::Wrap(v8_attr_value, Object());
}

void CJSObject::SetAttr(const std::string& name, py::object py_obj) const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  auto v8_attr_obj = CPythonObject::Wrap(std::move(py_obj));

  if (!Object()->Set(v8_context, v8_attr_name, v8_attr_obj).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

void CJSObject::DelAttr(const std::string& name) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  CheckAttr(v8_attr_name);

  if (!Object()->Delete(v8_context, v8_attr_name).FromMaybe(false)) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

py::list CJSObject::GetAttrList() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  CPythonGIL python_gil;

  py::list attrs;

  if (v8u::executionTerminating(v8_isolate)) {
    return attrs;
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto props = Object()->GetPropertyNames(v8_context).ToLocalChecked();

  for (size_t i = 0; i < props->Length(); i++) {
    auto v8_i = v8::Integer::New(v8_isolate, i);
    auto v8_prop = props->Get(v8_context, v8_i).ToLocalChecked();
    attrs.append(CJSObject::Wrap(v8_prop));
  }

  if (v8_try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return attrs;
}

int CJSObject::GetIdentityHash() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  return Object()->GetIdentityHash();
}

CJSObjectPtr CJSObject::Clone() const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  return CJSObjectPtr(new CJSObject(Object()->Clone()));
}

bool CJSObject::Contains(const std::string& name) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  bool found = Object()->Has(context, v8u::toString(name)).ToChecked();

  if (try_catch.HasCaught()) {
    CJSException::ThrowIf(isolate, try_catch);
  }

  return found;
}

bool CJSObject::Equals(const CJSObjectPtr& other) const {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  return other.get() && Object()->Equals(context, other->Object()).ToChecked();
}

void CJSObject::Dump(std::ostream& os) const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
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

py::object CJSObject::ToPythonInt() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->Int32Value(v8_context).ToChecked();
  return py::cast(val);
}

py::object CJSObject::ToPythonFloat() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_v8_obj.IsEmpty()) {
    throw CJSException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->NumberValue(v8_context).ToChecked();
  return py::cast(val);
}

py::object CJSObject::ToPythonBool() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);

  bool val = false;
  if (!m_v8_obj.IsEmpty()) {
    val = Object()->BooleanValue(v8_isolate);
  }

  return py::cast(val);
}

py::object CJSObject::ToPythonStr() const {
  std::stringstream ss;
  ss << *this;
  return py::cast(ss.str());
}

py::object CJSObject::Wrap(v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_self) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8_val.IsEmpty() || v8_val->IsNull() || v8_val->IsUndefined()) {
    return py::none();
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
    auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_val.As<v8::String>());
    return py::str(*v8_utf, v8_utf.length());
  }
  if (v8_val->IsStringObject()) {
    auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_val.As<v8::StringObject>()->ValueOf());
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
  return Wrap(v8_obj, v8_self);
}

py::object CJSObject::Wrap(v8::Local<v8::Object> v8_obj, v8::Local<v8::Object> v8_self) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8_obj.IsEmpty()) {
    return py::none();
  }
  if (v8_obj->IsArray()) {
    auto v8_array = v8_obj.As<v8::Array>();
    return Wrap(CJSObjectArrayPtr(new CJSObjectArray(v8_array)));
  }

  if (CPythonObject::IsWrapped2(v8_obj)) {
    return CPythonObject::GetWrapper2(v8_obj);
  }

  // TODO: CLJS support
  //
  //  auto wrapperHint = getWrapperHint(v8_obj);
  //  if (wrapperHint != kWrapperHintNone) {
  //    if (wrapperHint == kWrapperHintCCLJSIIterableIterator) {
  //      auto obj = new CCLJSIIterableIterator(v8_obj);
  //      return Wrap(obj);
  //    }
  //  }
  //
  //  if (isCLJSType(v8_obj)) {
  //    auto obj = new CJSObjectCLJS(v8_obj);
  //    return Wrap(obj);
  //  }

  if (v8_obj->IsFunction()) {
    auto v8_fn = v8_obj.As<v8::Function>();
    return Wrap(CJSObjectFunctionPtr(new CJSObjectFunction(v8_self, v8_fn)));
  }

  return Wrap(CJSObjectPtr(new CJSObject(v8_obj)));
}

py::object CJSObject::Wrap(const CJSObjectPtr& obj) {
  CPythonGIL python_gil;
  auto v8_isolate = v8::Isolate::GetCurrent();

  if (v8u::executionTerminating(v8_isolate)) {
    return py::none();
  }

  return py::cast(obj);
}

v8::Local<v8::Object> CJSObject::Object() const {
  return v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), m_v8_obj);
}
