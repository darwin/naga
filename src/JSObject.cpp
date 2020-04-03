#include "JSObject.h"
#include "JSObjectArray.h"
#include "JSObjectFunction.h"
#include "JSObjectNull.h"
#include "JSObjectUndefined.h"
#include "JSObjectCLJS.h"

#include "PythonObject.h"
#include "PythonDateTime.h"
#include "PythonGIL.h"
#include "Exception.h"

#define TERMINATE_EXECUTION_CHECK(returnValue)                         \
  if (v8::Isolate::GetCurrent()->IsExecutionTerminating()) {           \
    ::PyErr_Clear();                                                   \
    ::PyErr_SetString(PyExc_RuntimeError, "execution is terminating"); \
    return returnValue;                                                \
  }

#define CHECK_V8_CONTEXT()                                                                   \
  if (v8::Isolate::GetCurrent()->GetCurrentContext().IsEmpty()) {                            \
    throw CJavascriptException("Javascript object out of context", PyExc_UnboundLocalError); \
  }

std::ostream& operator<<(std::ostream& os, const CJSObject& obj) {
  obj.Dump(os);

  return os;
}

void CJSObject::Expose(void) {
  py::class_<CJSObject, boost::noncopyable>("JSObject", py::no_init)
      .def("__getattr__", &CJSObject::GetAttr)
      .def("__setattr__", &CJSObject::SetAttr)
      .def("__delattr__", &CJSObject::DelAttr)

      .def("__hash__", &CJSObject::GetIdentityHash)
      .def("clone", &CJSObject::Clone, "Clone the object.")
      .def("__dir__", &CJSObject::GetAttrList)

      // Emulating dict object
      .def("keys", &CJSObject::GetAttrList, "Get a list of the object attributes.")

      .def("__getitem__", &CJSObject::GetAttr)
      .def("__setitem__", &CJSObject::SetAttr)
      .def("__delitem__", &CJSObject::DelAttr)

      .def("__contains__", &CJSObject::Contains)

      .def(int_(py::self))
      .def(float_(py::self))
      .def(str(py::self))

      .def("__bool__", &CJSObject::operator bool)
      .def("__eq__", &CJSObject::Equals)
      .def("__ne__", &CJSObject::Unequals)

      .def("create", &CJavascriptFunction::CreateWithArgs,
           (py::arg("constructor"), py::arg("arguments") = py::tuple(), py::arg("propertiesObject") = py::dict()),
           "Creates a new object with the specified prototype object and properties.")
      .staticmethod("create");

  py::class_<CJavascriptNull, py::bases<CJSObject>, boost::noncopyable>("JSNull")
      .def("__bool__", &CJavascriptNull::nonzero)
      .def("__str__", &CJavascriptNull::str);

  py::class_<CJavascriptUndefined, py::bases<CJSObject>, boost::noncopyable>("JSUndefined")
      .def("__bool__", &CJavascriptUndefined::nonzero)
      .def("__str__", &CJavascriptUndefined::str);

  py::class_<CJavascriptArray, py::bases<CJSObject>, boost::noncopyable>("JSArray", py::no_init)
      .def(py::init<py::object>())

      .def("__len__", &CJavascriptArray::Length)

      .def("__getitem__", &CJavascriptArray::GetItem)
      .def("__setitem__", &CJavascriptArray::SetItem)
      .def("__delitem__", &CJavascriptArray::DelItem)

      .def("__iter__", py::range(&CJavascriptArray::begin, &CJavascriptArray::end))

      .def("__contains__", &CJavascriptArray::Contains);

  py::class_<CJavascriptFunction, py::bases<CJSObject>, boost::noncopyable>("JSFunction", py::no_init)
      .def("__call__", py::raw_function(&CJavascriptFunction::CallWithArgs))

      .def("apply", &CJavascriptFunction::ApplyJavascript,
           (py::arg("self"), py::arg("args") = py::list(), py::arg("kwds") = py::dict()),
           "Performs a function call using the parameters.")
      .def("apply", &CJavascriptFunction::ApplyPython,
           (py::arg("self"), py::arg("args") = py::list(), py::arg("kwds") = py::dict()),
           "Performs a function call using the parameters.")
      .def("invoke", &CJavascriptFunction::Invoke, (py::arg("args") = py::list(), py::arg("kwds") = py::dict()),
           "Performs a binding method call using the parameters.")

      .def("setName", &CJavascriptFunction::SetName)

      .add_property("name", &CJavascriptFunction::GetName, &CJavascriptFunction::SetName, "The name of function")
      .add_property("owner", &CJavascriptFunction::GetOwner)

      .add_property("linenum", &CJavascriptFunction::GetLineNumber, "The line number of function in the script")
      .add_property("colnum", &CJavascriptFunction::GetColumnNumber, "The column number of function in the script")
      .add_property("resname", &CJavascriptFunction::GetResourceName, "The resource name of script")
      .add_property("inferredname", &CJavascriptFunction::GetInferredName,
                    "Name inferred from variable or property assignment of this function")
      .add_property("lineoff", &CJavascriptFunction::GetLineOffset, "The line offset of function in the script")
      .add_property("coloff", &CJavascriptFunction::GetColumnOffset, "The column offset of function in the script");
  py::objects::class_value_wrapper<
      std::shared_ptr<CJSObject>,
      py::objects::make_ptr_instance<CJSObject,
                                     py::objects::pointer_holder<std::shared_ptr<CJSObject>, CJSObject> > >();

  exposeCLJSTypes();
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

py::object CJSObject::GetAttr(const std::string& name) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::String> attr_name = ToString(name);

  CheckAttr(attr_name);

  v8::Local<v8::Value> attr_value = Object()->Get(context, attr_name).ToLocalChecked();

  if (attr_value.IsEmpty())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return CJSObject::Wrap(attr_value, Object());
}

void CJSObject::SetAttr(const std::string& name, py::object value) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::String> attr_name = ToString(name);
  v8::Local<v8::Value> attr_obj = CPythonObject::Wrap(value);

  if (!Object()->Set(context, attr_name, attr_obj).FromMaybe(false)) {
    CJavascriptException::ThrowIf(isolate, try_catch);
  }
}

void CJSObject::DelAttr(const std::string& name) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::String> attr_name = ToString(name);

  CheckAttr(attr_name);

  if (!Object()->Delete(context, attr_name).FromMaybe(false))
    CJavascriptException::ThrowIf(isolate, try_catch);
}

py::list CJSObject::GetAttrList(void) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  CPythonGIL python_gil;

  py::list attrs;

  TERMINATE_EXECUTION_CHECK(attrs);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::Array> props = Object()->GetPropertyNames(context).ToLocalChecked();

  for (size_t i = 0; i < props->Length(); i++) {
    attrs.append(CJSObject::Wrap(props->Get(context, v8::Integer::New(isolate, i)).ToLocalChecked()));
  }

  if (try_catch.HasCaught())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return attrs;
}

int CJSObject::GetIdentityHash(void) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return Object()->GetIdentityHash();
}

CJSObjectPtr CJSObject::Clone(void) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return CJSObjectPtr(new CJSObject(Object()->Clone()));
}

bool CJSObject::Contains(const std::string& name) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  bool found = Object()->Has(context, ToString(name)).ToChecked();

  if (try_catch.HasCaught())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return found;
}

bool CJSObject::Equals(CJSObjectPtr other) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  return other.get() && Object()->Equals(context, other->Object()).ToChecked();
}

void CJSObject::Dump(std::ostream& os) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (m_obj.IsEmpty())
    os << "None";
  else if (Object()->IsInt32())
    os << Object()->Int32Value(context).ToChecked();
  else if (Object()->IsNumber())
    os << Object()->NumberValue(context).ToChecked();
  else if (Object()->IsBoolean())
    os << Object()->BooleanValue(isolate);
  else if (Object()->IsNull())
    os << "None";
  else if (Object()->IsUndefined())
    os << "N/A";
  else if (Object()->IsString())
    os << *v8::String::Utf8Value(isolate, v8::Local<v8::String>::Cast(Object()));
  else {
    v8::MaybeLocal<v8::String> s = Object()->ToString(context);
    if (s.IsEmpty())
      s = Object()->ObjectProtoToString(context);

    if (!s.IsEmpty())
      os << *v8::String::Utf8Value(isolate, s.ToLocalChecked());
  }
}

CJSObject::operator long() const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (m_obj.IsEmpty())
    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);

  return Object()->Int32Value(context).ToChecked();
}

CJSObject::operator double() const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  if (m_obj.IsEmpty())
    throw CJavascriptException("argument must be a string or a number, not 'NoneType'", ::PyExc_TypeError);

  return Object()->NumberValue(context).ToChecked();
}

CJSObject::operator bool() const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  if (m_obj.IsEmpty())
    return false;

  return Object()->BooleanValue(isolate);
}

py::object CJSObject::Wrap(v8::Local<v8::Value> value, v8::Local<v8::Object> self) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  assert(isolate->InContext());

  v8::HandleScope handle_scope(isolate);

  if (value.IsEmpty() || value->IsNull() || value->IsUndefined()) {
    return py::object();
  }
  if (value->IsTrue()) {
    return py::object(py::handle<>(py::borrowed(Py_True)));
  }
  if (value->IsFalse()) {
    return py::object(py::handle<>(py::borrowed(Py_False)));
  }
  if (value->IsInt32()) {
    return py::object(value->Int32Value(isolate->GetCurrentContext()).ToChecked());
  }
  if (value->IsString()) {
    v8::String::Utf8Value str(isolate, v8::Local<v8::String>::Cast(value));

    return py::str(*str, str.length());
  }
  if (value->IsStringObject()) {
    v8::String::Utf8Value str(isolate, value.As<v8::StringObject>()->ValueOf());

    return py::str(*str, str.length());
  }
  if (value->IsBoolean()) {
    return py::object(py::handle<>(py::borrowed(value->BooleanValue(isolate) ? Py_True : Py_False)));
  }
  if (value->IsBooleanObject()) {
    return py::object(
        py::handle<>(py::borrowed(value.As<v8::BooleanObject>()->BooleanValue(isolate) ? Py_True : Py_False)));
  }
  if (value->IsNumber()) {
    return py::object(py::handle<>(::PyFloat_FromDouble(value->NumberValue(isolate->GetCurrentContext()).ToChecked())));
  }
  if (value->IsNumberObject()) {
    return py::object(py::handle<>(
        ::PyFloat_FromDouble(value.As<v8::NumberObject>()->NumberValue(isolate->GetCurrentContext()).ToChecked())));
  }
  if (value->IsDate()) {
    double n = v8::Local<v8::Date>::Cast(value)->NumberValue(isolate->GetCurrentContext()).ToChecked();

    time_t ts = (time_t)floor(n / 1000);

    tm* t = localtime(&ts);

    return pythonFromDateAndTime(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min,
                                                  t->tm_sec, ((long long)floor(n)) % 1000 * 1000);
  }

  return Wrap(value->ToObject(isolate->GetCurrentContext()).ToLocalChecked(), self);
}

py::object CJSObject::Wrap(v8::Local<v8::Object> obj, v8::Local<v8::Object> self) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  if (obj.IsEmpty()) {
    return py::object();
  }

  if (obj->IsArray()) {
    v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(obj);

    return Wrap(new CJavascriptArray(array));
  }

  if (CPythonObject::IsWrapped(obj)) {
    return CPythonObject::Unwrap(obj);
  }

  auto wrapperHint = getWrapperHint(obj);
  if (wrapperHint != kWrapperHintNone) {
    if (wrapperHint == kWrapperHintCCLJSIIterableIterator) {
      auto o = new CCLJSIIterableIterator(obj);
      return Wrap(o);
    }
  }

  if (isCLJSType(obj)) {
    auto o = new CCLJSType(obj);
    return Wrap(o);
  }

  if (obj->IsFunction()) {
    return Wrap(new CJavascriptFunction(self, v8::Local<v8::Function>::Cast(obj)));
  }

  return Wrap(new CJSObject(obj));
}

py::object CJSObject::Wrap(CJSObject* obj) {
  CPythonGIL python_gil;

  TERMINATE_EXECUTION_CHECK(py::object())

  return py::object(py::handle<>(boost::python::converter::shared_ptr_to_python<CJSObject>(CJSObjectPtr(obj))));
}