#include "Wrapper.h"
#include "Context.h"
#include "Isolate.h"
#include "PythonObjectWrapper.h"
#include "PythonDateTime.h"
#include "WrapperCLJS.h"

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

void CJavascriptArray::LazyConstructor(void) {
  if (!m_obj.IsEmpty())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Array> array;

  if (m_items.is_none()) {
    array = v8::Array::New(isolate, m_size);
  } else if (PyLong_CheckExact(m_items.ptr())) {
    m_size = PyLong_AsLong(m_items.ptr());
    array = v8::Array::New(isolate, m_size);
  } else if (PyList_Check(m_items.ptr())) {
    m_size = PyList_GET_SIZE(m_items.ptr());
    array = v8::Array::New(isolate, m_size);

    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
      auto py_obj = py::object(py::handle<>(py::borrowed(PyList_GET_ITEM(m_items.ptr(), i))));
      auto wrapped_obj = CPythonObject::Wrap(py_obj);
      array->Set(context, v8::Uint32::New(isolate, i), wrapped_obj).Check();
    }
  } else if (PyTuple_Check(m_items.ptr())) {
    m_size = PyTuple_GET_SIZE(m_items.ptr());
    array = v8::Array::New(isolate, m_size);

    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
      auto py_obj = py::object(py::handle<>(py::borrowed(PyTuple_GET_ITEM(m_items.ptr(), i))));
      auto wrapped_obj = CPythonObject::Wrap(py_obj);
      array->Set(context, v8::Uint32::New(isolate, i),wrapped_obj).Check();
    }
  } else if (PyGen_Check(m_items.ptr())) {
    array = v8::Array::New(isolate);

    py::object iter(py::handle<>(::PyObject_GetIter(m_items.ptr())));

    m_size = 0;
    PyObject* item = NULL;

    while (NULL != (item = ::PyIter_Next(iter.ptr()))) {
      auto py_obj = py::object(py::handle<>(py::borrowed(item)));
      auto wrapped_obj = CPythonObject::Wrap(py_obj);
      array->Set(context, v8::Uint32::New(isolate, m_size++), wrapped_obj).Check();
    }
  }

  m_obj.Reset(isolate, array);
}

size_t CJavascriptArray::Length(void) {
  LazyConstructor();

  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return v8::Local<v8::Array>::Cast(Object())->Length();
}

py::object CJavascriptArray::GetItem(py::object key) {
  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr())) {
    Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
    Py_ssize_t start, stop, step, sliceLen;

    if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      py::list slice;

      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, (uint32_t)idx)).ToLocalChecked();

        if (value.IsEmpty())
          CJavascriptException::ThrowIf(isolate, try_catch);

        slice.append(CJSObject::Wrap(value, Object()));
      }

      return std::move(slice);
    }
  } else if (PyLong_Check(key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());

    if (!Object()->Has(context, idx).ToChecked())
      return py::object();

    v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, idx)).ToLocalChecked();

    if (value.IsEmpty())
      CJavascriptException::ThrowIf(isolate, try_catch);

    return CJSObject::Wrap(value, Object());
  }

  throw CJavascriptException("list indices must be integers", ::PyExc_TypeError);
}

py::object CJavascriptArray::SetItem(py::object key, py::object value) {
  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr())) {
    PyObject* values = ::PySequence_Fast(value.ptr(), "can only assign an iterable");

    if (values) {
      Py_ssize_t itemSize = PySequence_Fast_GET_SIZE(value.ptr());
      PyObject** items = PySequence_Fast_ITEMS(value.ptr());

      Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      Py_ssize_t start, stop, step, sliceLen;

      PySlice_Unpack(key.ptr(), &start, &stop, &step);

      /*
       * If the slice start is greater than the array length we append null elements
       * to the tail of the array to fill the gap
       */
      if (start > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < start; idx++) {
          Object()->Set(context, (uint32_t)(arrayLen + idx), v8::Null(isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      }

      /*
       * If the slice stop is greater than the array length (which was potentially
       * modified by the previous check) we append null elements to the tail of the
       * array. This step guarantees that the length of the array will always be
       * greater or equal than stop
       */
      if (stop > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < stop; idx++) {
          Object()->Set(context, (uint32_t)idx, v8::Null(isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      }

      if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
        if (itemSize != sliceLen) {
          if (itemSize < sliceLen) {
            Py_ssize_t diff = sliceLen - itemSize;

            for (Py_ssize_t idx = start + itemSize; idx < arrayLen - diff; idx++) {
              auto js_obj = Object()->Get(context, (uint32_t)(idx + diff)).ToLocalChecked();
              Object()->Set(context, idx, js_obj).Check();
            }
            for (Py_ssize_t idx = arrayLen - 1; idx > arrayLen - diff - 1; idx--) {
              Object()->Delete(context, (uint32_t)idx).Check();
            }
          } else if (itemSize > sliceLen) {
            Py_ssize_t diff = itemSize - sliceLen;

            for (Py_ssize_t idx = arrayLen + diff - 1; idx > stop - 1; idx--) {
              auto js_obj = Object()->Get(context, (uint32_t)(idx - diff)).ToLocalChecked();
              Object()->Set(context, idx, js_obj).Check();
            }
          }
        }

        for (Py_ssize_t idx = 0; idx < itemSize; idx++) {
          auto py_obj = py::object(py::handle<>(py::borrowed(items[idx])));
          auto wrapped_obj = CPythonObject::Wrap(py_obj);
          Object()->Set(context, (uint32_t)(start + idx * step), wrapped_obj).Check();
        }
      }
    }
  } else if (PyLong_Check(key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());

    if (!Object()->Set(context, v8::Integer::New(isolate, idx), CPythonObject::Wrap(value)).ToChecked())
      CJavascriptException::ThrowIf(isolate, try_catch);
  }

  return value;
}

py::object CJavascriptArray::DelItem(py::object key) {
  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr())) {
    Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
    Py_ssize_t start, stop, step, sliceLen;

    if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        Object()->Delete(context, (uint32_t)idx).Check();
      }
    }

    return py::object();
  } else if (PyLong_Check(key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());

    py::object value;

    if (Object()->Has(context, idx).ToChecked())
      value = CJSObject::Wrap(Object()->Get(context, v8::Integer::New(isolate, idx)).ToLocalChecked(), Object());

    if (!Object()->Delete(context, idx).ToChecked())
      CJavascriptException::ThrowIf(isolate, try_catch);

    return value;
  }

  throw CJavascriptException("list indices must be integers", ::PyExc_TypeError);
}

bool CJavascriptArray::Contains(py::object item) {
  LazyConstructor();

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  for (size_t i = 0; i < Length(); i++) {
    if (Object()->Has(context, i).ToChecked()) {
      v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, i)).ToLocalChecked();

      if (try_catch.HasCaught())
        CJavascriptException::ThrowIf(isolate, try_catch);

      if (item == CJSObject::Wrap(value, Object())) {
        return true;
      }
    }
  }

  if (try_catch.HasCaught())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return false;
}

py::object CJavascriptFunction::CallWithArgs(py::tuple args, py::dict kwds) {
  size_t argc = ::PyTuple_Size(args.ptr());

  if (argc == 0)
    throw CJavascriptException("missed self argument", ::PyExc_TypeError);

  py::object self = args[0];
  py::extract<CJavascriptFunction&> extractor(self);

  if (!extractor.check())
    throw CJavascriptException("missed self argument", ::PyExc_TypeError);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  CHECK_V8_CONTEXT();

  v8::TryCatch try_catch(isolate);

  CJavascriptFunction& func = extractor();
  py::list argv(args.slice(1, py::_));

  return func.Call(func.Self(), argv, kwds);
}

py::object CJavascriptFunction::Call(v8::Local<v8::Object> self, py::list args, py::dict kwds) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  size_t args_count = ::PyList_Size(args.ptr()), kwds_count = ::PyMapping_Size(kwds.ptr());

  std::vector<v8::Local<v8::Value> > params(args_count + kwds_count);

  for (size_t i = 0; i < args_count; i++) {
    params[i] = CPythonObject::Wrap(args[i]);
  }

  py::list values = kwds.values();

  for (size_t i = 0; i < kwds_count; i++) {
    params[args_count + i] = CPythonObject::Wrap(values[i]);
  }

  v8::MaybeLocal<v8::Value> result;

  Py_BEGIN_ALLOW_THREADS

      result = func->Call(context, self.IsEmpty() ? isolate->GetCurrentContext()->Global() : self, params.size(),
                          params.empty() ? NULL : &params[0]);

  Py_END_ALLOW_THREADS

      if (result.IsEmpty()) CJavascriptException::ThrowIf(isolate, try_catch);

  return CJSObject::Wrap(result.ToLocalChecked());
}

py::object CJavascriptFunction::CreateWithArgs(CJavascriptFunctionPtr proto, py::tuple args, py::dict kwds) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  if (proto->Object().IsEmpty())
    throw CJavascriptException("Object prototype may only be an Object", ::PyExc_TypeError);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(proto->Object());

  size_t args_count = ::PyTuple_Size(args.ptr());

  std::vector<v8::Local<v8::Value> > params(args_count);

  for (size_t i = 0; i < args_count; i++) {
    params[i] = CPythonObject::Wrap(args[i]);
  }

  v8::Local<v8::Object> result;

  Py_BEGIN_ALLOW_THREADS

      result = func->NewInstance(context, params.size(), params.empty() ? NULL : &params[0]).ToLocalChecked();

  Py_END_ALLOW_THREADS

      if (result.IsEmpty()) CJavascriptException::ThrowIf(isolate, try_catch);

  size_t kwds_count = ::PyMapping_Size(kwds.ptr());
  py::list items = kwds.items();

  for (size_t i = 0; i < kwds_count; i++) {
    py::tuple item(items[i]);

    py::str key(item[0]);
    py::object value = item[1];

    result->Set(context, ToString(key), CPythonObject::Wrap(value)).Check();
  }

  return CJSObject::Wrap(result);
}

py::object CJavascriptFunction::ApplyJavascript(CJSObjectPtr self, py::list args, py::dict kwds) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return Call(self->Object(), args, kwds);
}

py::object CJavascriptFunction::ApplyPython(py::object self, py::list args, py::dict kwds) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  return Call(CPythonObject::Wrap(self)->ToObject(context).ToLocalChecked(), args, kwds);
}

py::object CJavascriptFunction::Invoke(py::list args, py::dict kwds) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return Call(Self(), args, kwds);
}

const std::string CJavascriptFunction::GetName(void) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetName()));

  return std::string(*name, name.length());
}

void CJavascriptFunction::SetName(const std::string& name) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  func->SetName(
      v8::String::NewFromUtf8(isolate, name.c_str(), v8::NewStringType::kNormal, name.size()).ToLocalChecked());
}

int CJavascriptFunction::GetLineNumber(void) const {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptLineNumber();
}

int CJavascriptFunction::GetColumnNumber(void) const {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptColumnNumber();
}

const std::string CJavascriptFunction::GetResourceName(void) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetScriptOrigin().ResourceName()));

  return std::string(*name, name.length());
}

const std::string CJavascriptFunction::GetInferredName(void) const {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(func->GetInferredName()));

  return std::string(*name, name.length());
}

int CJavascriptFunction::GetLineOffset(void) const {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceLineOffset()->Value();
}

int CJavascriptFunction::GetColumnOffset(void) const {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(Object());

  return func->GetScriptOrigin().ResourceColumnOffset()->Value();
}

py::object CJavascriptFunction::GetOwner(void) const {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  CHECK_V8_CONTEXT();

  return CJSObject::Wrap(Self());
}

ObjectTracer::ObjectTracer(v8::Local<v8::Value> handle, py::object* object)
    : m_handle(v8::Isolate::GetCurrent(), handle), m_object(object), m_living(GetLivingMapping()) {}

ObjectTracer::~ObjectTracer() {
  if (!m_handle.IsEmpty()) {
    Dispose();

    m_living->erase(m_object->ptr());
  }
}

void ObjectTracer::Dispose(void) {
  // m_handle.ClearWeak();
  m_handle.Reset();
}

ObjectTracer& ObjectTracer::Trace(v8::Local<v8::Value> handle, py::object* object) {
  std::unique_ptr<ObjectTracer> tracer(new ObjectTracer(handle, object));

  tracer->Trace();

  return *tracer.release();
}

void ObjectTracer::Trace(void) {
  // m_handle.SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);

  m_living->insert(std::make_pair(m_object->ptr(), this));
}

/*
void ObjectTracer::WeakCallback(const v8::WeakCallbackInfo<ObjectTracer>& info)
{
  std::unique_ptr<ObjectTracer> tracer(info.GetParameter());

  // assert(info.GetValue() == tracer->m_handle);
}
*/

LivingMap* ObjectTracer::GetLivingMapping(void) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::Context> ctxt = v8::Isolate::GetCurrent()->GetCurrentContext();

  v8::Local<v8::String> key = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__living__").ToLocalChecked();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(v8::Isolate::GetCurrent(), key);

  v8::MaybeLocal<v8::Value> value = ctxt->Global()->GetPrivate(ctxt, privateKey);

  if (!value.IsEmpty()) {
    auto val = value.ToLocalChecked();
    if (val->IsExternal()) {
      LivingMap* living = (LivingMap*)v8::External::Cast(*val)->Value();

      if (living)
        return living;
    }
  }

  std::unique_ptr<LivingMap> living(new LivingMap());

  ctxt->Global()->SetPrivate(ctxt, privateKey, v8::External::New(v8::Isolate::GetCurrent(), living.get()));

  ContextTracer::Trace(ctxt, living.get());

  return living.release();
}

v8::Local<v8::Value> ObjectTracer::FindCache(py::object obj) {
  LivingMap* living = GetLivingMapping();

  if (living) {
    LivingMap::const_iterator it = living->find(obj.ptr());

    if (it != living->end()) {
      return v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), it->second->m_handle);
    }
  }

  return v8::Local<v8::Value>();
}

ContextTracer::ContextTracer(v8::Local<v8::Context> ctxt, LivingMap* living)
    : m_ctxt(v8::Isolate::GetCurrent(), ctxt), m_living(living) {}

ContextTracer::~ContextTracer(void) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::Context> ctxt = v8::Isolate::GetCurrent()->GetCurrentContext();
  v8::Local<v8::String> key = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__living__").ToLocalChecked();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(v8::Isolate::GetCurrent(), key);

  Context()->Global()->DeletePrivate(ctxt, privateKey);

  for (LivingMap::const_iterator it = m_living->begin(); it != m_living->end(); it++) {
    std::unique_ptr<ObjectTracer> tracer(it->second);

    tracer->Dispose();
  }
}

void ContextTracer::WeakCallback(const v8::WeakCallbackInfo<ContextTracer>& info) {
  delete info.GetParameter();
}

void ContextTracer::Trace(v8::Local<v8::Context> ctxt, LivingMap* living) {
  ContextTracer* tracer = new ContextTracer(ctxt, living);

  tracer->Trace();
}

void ContextTracer::Trace(void) {
  m_ctxt.SetWeak(this, WeakCallback, v8::WeakCallbackType::kFinalizer);
}