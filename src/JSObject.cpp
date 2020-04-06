#include "JSObject.h"
#include "JSObjectNull.h"
#include "JSObjectUndefined.h"
#include "JSObjectArray.h"
#include "JSObjectFunction.h"
#include "JSObjectCLJS.h"

#include "PythonObject.h"
#include "PythonDateTime.h"
#include "PythonGIL.h"
#include "Exception.h"

static std::ostream& operator<<(std::ostream& os, const CJSObject& obj) {
  obj.Dump(os);
  return os;
}

void CJSObject::Expose(pb::module& m) {
  // clang-format off
  pb::class_<CJSObject, CJSObjectPtr>(m, "JSObject")
      .def("__getattr__", &CJSObject::GetAttr2)
      .def("__setattr__", &CJSObject::SetAttr2)
      .def("__delattr__", &CJSObject::DelAttr2)

      .def("__hash__", &CJSObject::GetIdentityHash)
      .def("clone", &CJSObject::Clone,
           "Clone the object.")
      .def("__dir__", &CJSObject::GetAttrList2)

          // Emulating dict object
      .def("keys", &CJSObject::GetAttrList2,
           "Get a list of the object attributes.")

      .def("__getitem__", &CJSObject::GetAttr2)
      .def("__setitem__", &CJSObject::SetAttr2)
      .def("__delitem__", &CJSObject::DelAttr2)

      .def("__contains__", &CJSObject::Contains)

      .def("__int__", &CJSObject::ToPythonInt2)
      .def("__float__", &CJSObject::ToPythonFloat2)
      .def("__str__", &CJSObject::ToPythonStr2)

      .def("__bool__", &CJSObject::ToPythonBool2)
      .def("__eq__", &CJSObject::Equals)
      .def("__ne__", &CJSObject::Unequals)

      .def_static("create", &CJSObjectFunction::CreateWithArgs2, pb::arg("constructor"),
                  pb::arg("arguments") = pb::tuple(), pb::arg("propertiesObject") = pb::dict(),
                  "Creates a new object with the specified prototype object and properties.");

  //  py::class_<CJSObject, boost::noncopyable>("JSObject", py::no_init)
  //      .def("__getattr__", &CJSObject::GetAttr)
  //      .def("__setattr__", &CJSObject::SetAttr)
  //      .def("__delattr__", &CJSObject::DelAttr)
  //
  //      .def("__hash__", &CJSObject::GetIdentityHash)
  //      .def("clone", &CJSObject::Clone, "Clone the object.")
  //      .def("__dir__", &CJSObject::GetAttrList)
  //
  //      // Emulating dict object
  //      .def("keys", &CJSObject::GetAttrList, "Get a list of the object attributes.")
  //
  //      .def("__getitem__", &CJSObject::GetAttr)
  //      .def("__setitem__", &CJSObject::SetAttr)
  //      .def("__delitem__", &CJSObject::DelAttr)
  //
  //      .def("__contains__", &CJSObject::Contains)
  //
  //      .def("__int__", &CJSObject::ToPythonInt)
  //      .def("__float__", &CJSObject::ToPythonFloat)
  //      .def("__str__", &CJSObject::ToPythonStr)
  //
  //      .def("__bool__", &CJSObject::ToPythonBool)
  //      .def("__eq__", &CJSObject::Equals)
  //      .def("__ne__", &CJSObject::Unequals)
  //
  //      .def("create", &CJSObjectFunction::CreateWithArgs,
  //           (py::arg("constructor"), py::arg("arguments") = py::tuple(), py::arg("propertiesObject") = py::dict()),
  //           "Creates a new object with the specified prototype object and properties.")
  //      .staticmethod("create");

  // CJSObjectPtr, CJSObject
  pb::class_<CJSObjectNull, CJSObjectNullPtr, CJSObject>(m, "JSNull")
      .def(pb::init<>())
      .def("__bool__", &CJSObjectNull::nonzero)
            .def("__str__", &CJSObjectNull::str)
      ;

  //  py::class_<CJSObjectNull, py::bases<CJSObject>, boost::noncopyable>("JSNull")
  //      .def("__bool__", &CJSObjectNull::nonzero)
  //      .def("__str__", &CJSObjectNull::str);

  pb::class_<CJSObjectUndefined, CJSObjectUndefinedPtr, CJSObject>(m, "JSUndefined")
      .def(pb::init<>())
      .def("__bool__", &CJSObjectUndefined::nonzero)
      .def("__str__", &CJSObjectUndefined::str);

  //  py::class_<CJSObjectUndefined, py::bases<CJSObject>, boost::noncopyable>("JSUndefined")
  //      .def("__bool__", &CJSObjectUndefined::nonzero)
  //      .def("__str__", &CJSObjectUndefined::str);

  pb::class_<CJSObjectArray, CJSObjectArrayPtr, CJSObject>(m, "JSArray")
      .def(pb::init<pb::object>())

      .def("__len__", &CJSObjectArray::Length)

      .def("__getitem__", &CJSObjectArray::GetItem)
      .def("__setitem__", &CJSObjectArray::SetItem)
      .def("__delitem__", &CJSObjectArray::DelItem)

      // TODO:      .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)

      .def("__contains__", &CJSObjectArray::Contains);

  //  py::class_<CJSObjectArray, py::bases<CJSObject>, boost::noncopyable>("JSArray", py::no_init)
  //      .def(py::init<py::object>())
  //
  //      .def("__len__", &CJSObjectArray::Length)
  //
  //      .def("__getitem__", &CJSObjectArray::GetItem)
  //      .def("__setitem__", &CJSObjectArray::SetItem)
  //      .def("__delitem__", &CJSObjectArray::DelItem)
  //
  //      .def("__iter__", py::range(&CJSObjectArray::begin, &CJSObjectArray::end))
  //
  //      .def("__contains__", &CJSObjectArray::Contains);

  pb::class_<CJSObjectFunction, CJSObjectFunctionPtr, CJSObject>(m, "JSFunction")
      .def("__call__", &CJSObjectFunction::CallWithArgs2)

      .def("apply", &CJSObjectFunction::ApplyJavascript2,
           pb::arg("self"),
           pb::arg("args") = pb::list(),
           pb::arg("kwds") = pb::dict(),
           "Performs a function call using the parameters.")
      .def("apply", &CJSObjectFunction::ApplyPython2,
           pb::arg("self"),
           pb::arg("args") = pb::list(),
           pb::arg("kwds") = pb::dict(),
           "Performs a function call using the parameters.")
      .def("invoke", &CJSObjectFunction::Invoke2,
           pb::arg("args") = pb::list(),
           pb::arg("kwds") = pb::dict(),
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

  //  py::class_<CJSObjectFunction, py::bases<CJSObject>, boost::noncopyable>("JSFunction", py::no_init)
  //      .def("__call__", py::raw_function(&CJSObjectFunction::CallWithArgs))
  //
  //      .def("apply", &CJSObjectFunction::ApplyJavascript,
  //           (py::arg("self"), py::arg("args") = py::list(), py::arg("kwds") = py::dict()),
  //           "Performs a function call using the parameters.")
  //      .def("apply", &CJSObjectFunction::ApplyPython,
  //           (py::arg("self"), py::arg("args") = py::list(), py::arg("kwds") = py::dict()),
  //           "Performs a function call using the parameters.")
  //      .def("invoke", &CJSObjectFunction::Invoke, (py::arg("args") = py::list(), py::arg("kwds") = py::dict()),
  //           "Performs a binding method call using the parameters.")
  //
  //      .def("setName", &CJSObjectFunction::SetName)
  //
  //      .add_property("name", &CJSObjectFunction::GetName, &CJSObjectFunction::SetName, "The name of function")
  //      .add_property("owner", &CJSObjectFunction::GetOwner)
  //
  //      .add_property("linenum", &CJSObjectFunction::GetLineNumber, "The line number of function in the script")
  //      .add_property("colnum", &CJSObjectFunction::GetColumnNumber, "The column number of function in the script")
  //      .add_property("resname", &CJSObjectFunction::GetResourceName, "The resource name of script")
  //      .add_property("inferredname", &CJSObjectFunction::GetInferredName,
  //                    "Name inferred from variable or property assignment of this function")
  //      .add_property("lineoff", &CJSObjectFunction::GetLineOffset, "The line offset of function in the script")
  //      .add_property("coloff", &CJSObjectFunction::GetColumnOffset, "The column offset of function in the script");

  //  py::objects::class_value_wrapper<
  //      std::shared_ptr<CJSObject>,
  //      py::objects::make_ptr_instance<CJSObject,
  //                                     py::objects::pointer_holder<std::shared_ptr<CJSObject>, CJSObject> > >();

  // exposeCLJSTypes();
  // clang-format on
}

void CJSObject::CheckAttr(v8::Local<v8::String> name) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  assert(isolate->InContext());

  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (!Object()->Has(context, name).FromMaybe(false)) {
    std::ostringstream msg;

    msg << "'" << *v8::String::Utf8Value(isolate, Object()->ObjectProtoToString(context).ToLocalChecked())
        << "' object has no attribute '" << *v8::String::Utf8Value(isolate, name) << "'";

    throw CJavascriptException(msg.str(), ::PyExc_AttributeError);
  }
}

// py::object CJSObject::GetAttr(const std::string& name) {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8u::checkContext(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  v8::Local<v8::String> attr_name = v8u::toString(name);
//
//  CheckAttr(attr_name);
//
//  v8::Local<v8::Value> attr_value = Object()->Get(context, attr_name).ToLocalChecked();
//
//  if (attr_value.IsEmpty())
//    CJavascriptException::ThrowIf(isolate, try_catch);
//
//  return CJSObject::Wrap(attr_value, Object());
//}

pb::object CJSObject::GetAttr2(const std::string& name) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  CheckAttr(v8_attr_name);

  auto v8_attr_value = Object()->Get(v8_context, v8_attr_name).ToLocalChecked();
  if (v8_attr_value.IsEmpty()) {
    CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return CJSObject::Wrap(v8_attr_value, Object());
}

// void CJSObject::SetAttr(const std::string& name, py::object value) {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8u::checkContext(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  v8::Local<v8::String> attr_name = v8u::toString(name);
//  v8::Local<v8::Value> attr_obj = CPythonObject::Wrap(value);
//
//  if (!Object()->Set(context, attr_name, attr_obj).FromMaybe(false)) {
//    CJavascriptException::ThrowIf(isolate, try_catch);
//  }
//}

void CJSObject::SetAttr2(const std::string& name, pb::object py_obj) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  auto v8_attr_obj = CPythonObject::Wrap(py_obj);

  if (!Object()->Set(v8_context, v8_attr_name, v8_attr_obj).FromMaybe(false)) {
    CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

// void CJSObject::DelAttr(const std::string& name) {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8u::checkContext(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  v8::Local<v8::String> attr_name = v8u::toString(name);
//
//  CheckAttr(attr_name);
//
//  if (!Object()->Delete(context, attr_name).FromMaybe(false))
//    CJavascriptException::ThrowIf(isolate, try_catch);
//}

void CJSObject::DelAttr2(const std::string& name) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  auto v8_attr_name = v8u::toString(name);
  CheckAttr(v8_attr_name);

  if (!Object()->Delete(v8_context, v8_attr_name).FromMaybe(false)) {
    CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
  }
}

// py::list CJSObject::GetAttrList() {
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//
//  v8u::checkContext(isolate);
//
//  CPythonGIL python_gil;
//
//  py::list attrs;
//
//  if (v8u::executionTerminating(isolate)) {
//    return attrs;
//  }
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  v8::Local<v8::Array> props = Object()->GetPropertyNames(context).ToLocalChecked();
//
//  for (size_t i = 0; i < props->Length(); i++) {
//    attrs.append(CJSObject::Wrap(props->Get(context, v8::Integer::New(isolate, i)).ToLocalChecked()));
//  }
//
//  if (try_catch.HasCaught())
//    CJavascriptException::ThrowIf(isolate, try_catch);
//
//  return attrs;
//}

pb::list CJSObject::GetAttrList2() {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  v8u::checkContext(v8_isolate);

  CPythonGIL python_gil;

  pb::list attrs;

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
    CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return attrs;
}

int CJSObject::GetIdentityHash() {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  return Object()->GetIdentityHash();
}

CJSObjectPtr CJSObject::Clone() {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  return CJSObjectPtr(new CJSObject(Object()->Clone()));
}

bool CJSObject::Contains(const std::string& name) {
  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  bool found = Object()->Has(context, v8u::toString(name)).ToChecked();

  if (try_catch.HasCaught())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return found;
}

bool CJSObject::Equals(CJSObjectPtr other) const {
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

  if (m_obj.IsEmpty())
    os << "None";  // TODO: this should be something different than "None"
  else if (Object()->IsInt32())
    os << Object()->Int32Value(v8_context).ToChecked();
  else if (Object()->IsNumber())
    os << Object()->NumberValue(v8_context).ToChecked();
  else if (Object()->IsBoolean())
    os << Object()->BooleanValue(v8_isolate);
  else if (Object()->IsNull())
    os << "None";
  else if (Object()->IsUndefined())
    os << "N/A";
  else if (Object()->IsString())
    os << *v8::String::Utf8Value(v8_isolate, v8::Local<v8::String>::Cast(Object()));
  else {
    v8::MaybeLocal<v8::String> s = Object()->ToString(v8_context);
    if (s.IsEmpty())
      s = Object()->ObjectProtoToString(v8_context);

    if (!s.IsEmpty())
      os << *v8::String::Utf8Value(v8_isolate, s.ToLocalChecked());
  }
}

// py::object CJSObject::ToPythonInt() const {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  if (m_obj.IsEmpty()) {
//    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);
//  }
//
//  auto val = Object()->Int32Value(context).ToChecked();
//  return py::object(val);
//}

pb::object CJSObject::ToPythonInt2() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_obj.IsEmpty()) {
    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", PyExc_TypeError);
  }

  auto val = Object()->Int32Value(v8_context).ToChecked();
  return pb::cast(val);
}

// py::object CJSObject::ToPythonFloat() const {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  if (m_obj.IsEmpty()) {
//    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);
//  }
//
//  auto val = Object()->NumberValue(context).ToChecked();
//  return py::object(val);
//}

pb::object CJSObject::ToPythonFloat2() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  if (m_obj.IsEmpty()) {
    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);
  }

  auto val = Object()->NumberValue(v8_context).ToChecked();
  return pb::cast(val);
}

// py::object CJSObject::ToPythonBool() const {
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  bool val = false;
//  if (!m_obj.IsEmpty()) {
//    val = Object()->BooleanValue(isolate);
//  }
//
//  if (val) {
//    return py::object(py::handle<>(py::borrowed(Py_False)));
//  } else {
//    return py::object(py::handle<>(py::borrowed(Py_True)));
//  }
//}

pb::object CJSObject::ToPythonBool2() const {
  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);

  bool val = false;
  if (!m_obj.IsEmpty()) {
    val = Object()->BooleanValue(v8_isolate);
  }

  return pb::cast(val);
}

// py::object CJSObject::ToPythonStr() const {
//  std::stringstream ss;
//  Dump(ss);
//  return py::object(ss.str());
//}

pb::object CJSObject::ToPythonStr2() const {
  std::stringstream ss;
  ss << *this;
  return pb::cast(ss.str());
}

// py::object CJSObject::Wrap(v8::Local<v8::Value> value, v8::Local<v8::Object> self) {
//  auto isolate = v8::Isolate::GetCurrent();
//  assert(isolate->InContext());
//
//  v8::HandleScope handle_scope(isolate);
//
//  if (value.IsEmpty() || value->IsNull() || value->IsUndefined()) {
//    return py::object();
//  }
//  if (value->IsTrue()) {
//    return py::object(py::handle<>(py::borrowed(Py_True)));
//  }
//  if (value->IsFalse()) {
//    return py::object(py::handle<>(py::borrowed(Py_False)));
//  }
//  if (value->IsInt32()) {
//    return py::object(value->Int32Value(isolate->GetCurrentContext()).ToChecked());
//  }
//  if (value->IsString()) {
//    v8::String::Utf8Value str(isolate, v8::Local<v8::String>::Cast(value));
//
//    return py::str(*str, str.length());
//  }
//  if (value->IsStringObject()) {
//    v8::String::Utf8Value str(isolate, value.As<v8::StringObject>()->ValueOf());
//
//    return py::str(*str, str.length());
//  }
//  if (value->IsBoolean()) {
//    return py::object(py::handle<>(py::borrowed(value->BooleanValue(isolate) ? Py_True : Py_False)));
//  }
//  if (value->IsBooleanObject()) {
//    return py::object(
//        py::handle<>(py::borrowed(value.As<v8::BooleanObject>()->BooleanValue(isolate) ? Py_True : Py_False)));
//  }
//  if (value->IsNumber()) {
//    return
//    py::object(py::handle<>(::PyFloat_FromDouble(value->NumberValue(isolate->GetCurrentContext()).ToChecked())));
//  }
//  if (value->IsNumberObject()) {
//    return py::object(py::handle<>(
//        ::PyFloat_FromDouble(value.As<v8::NumberObject>()->NumberValue(isolate->GetCurrentContext()).ToChecked())));
//  }
//  if (value->IsDate()) {
//    double n = v8::Local<v8::Date>::Cast(value)->NumberValue(isolate->GetCurrentContext()).ToChecked();
//
//    time_t ts = (time_t)floor(n / 1000);
//
//    tm* t = localtime(&ts);
//
//    return pythonFromDateAndTime(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
//                                 ((long long)floor(n)) % 1000 * 1000);
//  }
//
//  return Wrap(value->ToObject(isolate->GetCurrentContext()).ToLocalChecked(), self);
//}

pb::object CJSObject::Wrap(v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_self) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8_val.IsEmpty() || v8_val->IsNull() || v8_val->IsUndefined()) {
    return pb::none();
  }
  if (v8_val->IsTrue()) {
    return pb::bool_(true);
  }
  if (v8_val->IsFalse()) {
    return pb::bool_(false);
  }
  if (v8_val->IsInt32()) {
    auto int32 = v8_val->Int32Value(v8_isolate->GetCurrentContext()).ToChecked();
    return pb::int_(int32);
  }
  if (v8_val->IsString()) {
    auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_val.As<v8::String>());
    return pb::str(*v8_utf, v8_utf.length());
  }
  if (v8_val->IsStringObject()) {
    auto v8_utf = v8u::toUtf8Value(v8_isolate, v8_val.As<v8::StringObject>()->ValueOf());
    return pb::str(*v8_utf, v8_utf.length());
  }
  if (v8_val->IsBoolean()) {
    bool val = v8_val->BooleanValue(v8_isolate);
    return pb::bool_(val);
  }
  if (v8_val->IsBooleanObject()) {
    auto val = v8_val.As<v8::BooleanObject>()->BooleanValue(v8_isolate);
    return pb::bool_(val);
  }
  if (v8_val->IsNumber()) {
    auto val = v8_val->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    return pb::float_(val);
  }
  if (v8_val->IsNumberObject()) {
    auto val = v8_val.As<v8::NumberObject>()->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    return pb::float_(val);
  }
  if (v8_val->IsDate()) {
    auto val = v8_val.As<v8::Date>()->NumberValue(v8_isolate->GetCurrentContext()).ToChecked();
    time_t ts = (time_t)floor(val / 1000);
    tm* t = localtime(&ts);
    return pythonFromDateAndTime2(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
                                  ((long long)floor(val)) % 1000 * 1000);
  }

  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_obj = v8_val->ToObject(v8_context).ToLocalChecked();
  return Wrap(v8_obj, v8_self);
}

// static pb::object Wrap(CJSObject* obj);
// static pb::object Wrap(v8::Local<v8::Object> v8_obj, v8::Local<v8::Object> v8_self);

// py::object CJSObject::Wrap(v8::Local<v8::Object> obj, v8::Local<v8::Object> self) {
//  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
//
//  if (obj.IsEmpty()) {
//    return py::object();
//  }
//
//  if (obj->IsArray()) {
//    v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(obj);
//
//    return Wrap(new CJSObjectArray(array));
//  }
//
////  if (CPythonObject::IsWrapped(obj)) {
////    return CPythonObject::GetWrapper(obj);
////  }
//
//  if (CPythonObject::IsWrapped2(obj)) {
//    auto py_obj = CPythonObject::GetWrapper2(obj);
//    return py::object(py::handle<>(py::borrowed(py_obj.ptr())));
//  }
//
//  //  auto wrapperHint = getWrapperHint(obj);
//  //  if (wrapperHint != kWrapperHintNone) {
//  //    if (wrapperHint == kWrapperHintCCLJSIIterableIterator) {
//  //      auto o = new CCLJSIIterableIterator(obj);
//  //      return Wrap(o);
//  //    }
//  //  }
//  //
//  //  if (isCLJSType(obj)) {
//  //    auto o = new CJSObjectCLJS(obj);
//  //    return Wrap(o);
//  //  }
//
//  if (obj->IsFunction()) {
//    std::cerr << "WRAPPING AS FN1" << "\n";
//    return Wrap(new CJSObjectFunction(self, v8::Local<v8::Function>::Cast(obj)));
//  }
//
//  return Wrap(new CJSObject(obj));
//}

pb::object CJSObject::Wrap(v8::Local<v8::Object> v8_obj, v8::Local<v8::Object> v8_self) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8_obj.IsEmpty()) {
    return pb::none();
  }
  if (v8_obj->IsArray()) {
    auto v8_array = v8_obj.As<v8::Array>();
    return Wrap(CJSObjectArrayPtr(new CJSObjectArray(v8_array)));
  }

  if (CPythonObject::IsWrapped2(v8_obj)) {
    return CPythonObject::GetWrapper2(v8_obj);
  }

  //  if (CPythonObject::IsWrapped(v8_obj)) {
  //    auto py_obj = CPythonObject::GetWrapper(v8_obj);
  //    return pb::reinterpret_borrow<pb::object>(py_obj.ptr());
  //  }

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

// py::object CJSObject::Wrap(CJSObject* obj) {
//  CPythonGIL python_gil;
//  auto isolate = v8::Isolate::GetCurrent();
//
//  if (v8u::executionTerminating(isolate)) {
//    return py::object();
//  }
//
//  return py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CJSObject>(CJSObjectPtr(obj))));
//}

pb::object CJSObject::Wrap(CJSObjectPtr obj) {
  CPythonGIL python_gil;
  auto v8_isolate = v8::Isolate::GetCurrent();

  if (v8u::executionTerminating(v8_isolate)) {
    return pb::none();
  }

  return pb::cast(obj);
}