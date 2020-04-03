#include "PythonObject.h"
#include "JSObject.h"
#include "JSObjectNull.h"
#include "JSObjectUndefined.h"
#include "Isolate.h"
#include "PythonDateTime.h"
#include "Tracer.h"
#include "PythonGIL.h"
#include "PythonExceptionGuard.h"

void CPythonObject::ThrowIf(v8::Isolate* isolate) {
  CPythonGIL python_gil;

  assert(PyErr_Occurred());

  v8::HandleScope handle_scope(isolate);

  PyObject *exc, *val, *trb;

  ::PyErr_Fetch(&exc, &val, &trb);
  ::PyErr_NormalizeException(&exc, &val, &trb);

  py::object type(py::handle<>(py::allow_null(exc))), value(py::handle<>(py::allow_null(val)));

  if (trb)
    py::decref(trb);

  std::string msg;

  if (::PyObject_HasAttrString(value.ptr(), "args")) {
    py::object args = value.attr("args");

    if (PyTuple_Check(args.ptr())) {
      for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(args.ptr()); i++) {
        py::extract<const std::string> extractor(args[i]);

        if (extractor.check())
          msg += extractor();
      }
    }
  } else if (::PyObject_HasAttrString(value.ptr(), "message")) {
    py::extract<const std::string> extractor(value.attr("message"));

    if (extractor.check())
      msg = extractor();
  } else if (val) {
    if (PyBytes_CheckExact(val)) {
      msg = PyBytes_AS_STRING(val);
    } else if (PyTuple_CheckExact(val)) {
      for (int i = 0; i < PyTuple_GET_SIZE(val); i++) {
        PyObject* item = PyTuple_GET_ITEM(val, i);

        if (item && PyBytes_CheckExact(item)) {
          msg = PyBytes_AS_STRING(item);
          break;
        }
      }
    }
  }

  v8::Local<v8::Value> error;

  if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_IndexError)) {
    error = v8::Exception::RangeError(
        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  } else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_AttributeError)) {
    error = v8::Exception::ReferenceError(
        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  } else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_SyntaxError)) {
    error = v8::Exception::SyntaxError(
        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  } else if (::PyErr_GivenExceptionMatches(type.ptr(), ::PyExc_TypeError)) {
    error = v8::Exception::TypeError(
        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  } else {
    error = v8::Exception::Error(
        v8::String::NewFromUtf8(isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked());
  }

  if (error->IsObject()) {
    v8::Local<v8::Context> ctxt = isolate->GetCurrentContext();

    v8::Local<v8::String> key_type = v8::String::NewFromUtf8(isolate, "exc_type").ToLocalChecked();
    v8::Local<v8::Private> privateKey_type = v8::Private::ForApi(isolate, key_type);

    v8::Local<v8::String> key_value = v8::String::NewFromUtf8(isolate, "exc_value").ToLocalChecked();
    v8::Local<v8::Private> privateKey_value = v8::Private::ForApi(isolate, key_value);

    error->ToObject(ctxt).ToLocalChecked()->SetPrivate(
        ctxt, privateKey_type, v8::External::New(isolate, ObjectTracer::Trace(error, new py::object(type)).Object()));
    error->ToObject(ctxt).ToLocalChecked()->SetPrivate(
        ctxt, privateKey_value, v8::External::New(isolate, ObjectTracer::Trace(error, new py::object(value)).Object()));
  }

  isolate->ThrowException(error);
}

CPythonObject::CPythonObject() {}

CPythonObject::~CPythonObject() {}

void CPythonObject::NamedGetter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  if (prop->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    info.GetReturnValue().Set(v8::Undefined(isolate));
    return;
  }
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Undefined(isolate));
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    v8::String::Utf8Value name(isolate, v8::Local<v8::String>::Cast(prop));

    if (PyGen_Check(obj.ptr())) {
      return v8::Undefined(isolate).As<v8::Value>();
    }

    if (*name == nullptr) {
      return v8::Undefined(isolate).As<v8::Value>();
    }

    PyObject* value = ::PyObject_GetAttrString(obj.ptr(), *name);

    if (!value) {
      if (PyErr_Occurred()) {
        if (::PyErr_ExceptionMatches(::PyExc_AttributeError)) {
          ::PyErr_Clear();
        } else {
          py::throw_error_already_set();
        }
      }

      if (::PyMapping_Check(obj.ptr()) && ::PyMapping_HasKeyString(obj.ptr(), *name)) {
        py::object result(py::handle<>(::PyMapping_GetItemString(obj.ptr(), *name)));

        if (!result.is_none()) {
          return Wrap(result);
        }
      }

      return v8::Local<v8::Value>();
    }

    py::object attr = py::object(py::handle<>(value));

    if (PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type)) {
      py::object getter = attr.attr("fget");

      if (getter.is_none()) {
        throw CJavascriptException("unreadable attribute", ::PyExc_AttributeError);
      }

      attr = getter();
    }

    return Wrap(attr);
  });

  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::NamedSetter(v8::Local<v8::Name> prop,
                                v8::Local<v8::Value> value,
                                const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  if (prop->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    info.GetReturnValue().Set(v8::Undefined(isolate));
    return;
  }
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Undefined(isolate));
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    v8::String::Utf8Value name(isolate, prop);
    py::object newval = CJSObject::Wrap(value);

    bool found = 1 == ::PyObject_HasAttrString(obj.ptr(), *name);

    if (::PyObject_HasAttrString(obj.ptr(), "__watchpoints__")) {
      py::dict watchpoints(obj.attr("__watchpoints__"));
      py::str propname(*name, name.length());

      if (watchpoints.has_key(propname)) {
        py::object watchhandler = watchpoints.get(propname);
        newval = watchhandler(propname, found ? obj.attr(propname) : py::object(), newval);
      }
    }

    if (!found && ::PyMapping_Check(obj.ptr())) {
      ::PyMapping_SetItemString(obj.ptr(), *name, newval.ptr());
    } else {
      if (found) {
        py::object attr = obj.attr(*name);

        if (PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type)) {
          py::object setter = attr.attr("fset");

          if (setter.is_none()) {
            throw CJavascriptException("can't set attribute", ::PyExc_AttributeError);
          }

          setter(newval);
          return value;
        }
      }
      obj.attr(*name) = newval;
    }

    return value;
  });

  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::NamedQuery(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Integer>& info) {
  auto isolate = info.GetIsolate();
  if (prop->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Integer>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    v8::String::Utf8Value name(isolate, prop);

    bool exists = PyGen_Check(obj.ptr()) || ::PyObject_HasAttrString(obj.ptr(), *name) ||
                  (::PyMapping_Check(obj.ptr()) && ::PyMapping_HasKeyString(obj.ptr(), *name));

    if (exists) {
      return v8::Integer::New(isolate, v8::None);
    } else {
      return v8::Local<v8::Integer>();
    }
  });

  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Integer>(); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::NamedDeleter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  auto isolate = info.GetIsolate();
  if (prop->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    v8::String::Utf8Value name(isolate, prop);

    if (!::PyObject_HasAttrString(obj.ptr(), *name) && ::PyMapping_Check(obj.ptr()) &&
        ::PyMapping_HasKeyString(obj.ptr(), *name)) {
      return v8::Boolean::New(isolate, -1 != ::PyMapping_DelItemString(obj.ptr(), *name));
    } else {
      py::object attr = obj.attr(*name);

      if (::PyObject_HasAttrString(obj.ptr(), *name) && PyObject_TypeCheck(attr.ptr(), &::PyProperty_Type)) {
        py::object deleter = attr.attr("fdel");

        if (deleter.is_none())
          throw CJavascriptException("can't delete attribute", ::PyExc_AttributeError);

        return v8::Boolean::New(isolate, py::extract<bool>(deleter()));
      } else {
        return v8::Boolean::New(isolate, -1 != ::PyObject_DelAttrString(obj.ptr(), *name));
      }
    }
  });

  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Boolean>(); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Array>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    py::list keys;
    bool filter_name = false;

    if (::PySequence_Check(obj.ptr())) {
      return v8::Local<v8::Array>();
    } else if (::PyMapping_Check(obj.ptr())) {
      keys = py::list(py::handle<>(PyMapping_Keys(obj.ptr())));
    } else if (PyGen_CheckExact(obj.ptr())) {
      py::object iter(py::handle<>(::PyObject_GetIter(obj.ptr())));

      PyObject* item = NULL;

      while (NULL != (item = ::PyIter_Next(iter.ptr()))) {
        keys.append(py::object(py::handle<>(item)));
      }
    } else {
      keys = py::list(py::handle<>(::PyObject_Dir(obj.ptr())));
      filter_name = true;
    }

    Py_ssize_t len = PyList_GET_SIZE(keys.ptr());
    v8::Local<v8::Array> result = v8::Array::New(isolate, len);
    for (Py_ssize_t i = 0; i < len; i++) {
      PyObject* item = PyList_GET_ITEM(keys.ptr(), i);

      if (filter_name && PyBytes_CheckExact(item)) {
        py::str name(py::handle<>(py::borrowed(item)));

        // FIXME Are there any methods to avoid such a dirty work?

        if (name.startswith("__") && name.endswith("__")) {
          continue;
        }
      }

      auto res = result->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8::Uint32::New(isolate, i),
                             Wrap(py::object(py::handle<>(py::borrowed(item)))));
      res.Check();
    }
    return result;
  });

  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Array>(); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Undefined(isolate));
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    if (PyGen_Check(obj.ptr())) {
      return v8::Undefined(isolate).As<v8::Value>();
    }

    if (::PySequence_Check(obj.ptr())) {
      if ((Py_ssize_t)index < ::PySequence_Size(obj.ptr())) {
        py::object ret(py::handle<>(::PySequence_GetItem(obj.ptr(), index)));

        return Wrap(ret);
      } else {
        return v8::Undefined(isolate).As<v8::Value>();
      }
    }

    if (::PyMapping_Check(obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);

      PyObject* value = ::PyMapping_GetItemString(obj.ptr(), buf);

      if (!value) {
        py::long_ key(index);

        value = ::PyObject_GetItem(obj.ptr(), key.ptr());
      }

      if (value) {
        return Wrap(py::object(py::handle<>(value)));
      } else {
        return v8::Undefined(isolate).As<v8::Value>();
      }
    }

    return v8::Undefined(isolate).As<v8::Value>();
  });

  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::IndexedSetter(uint32_t index,
                                  v8::Local<v8::Value> value,
                                  const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Undefined(isolate));
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    if (::PySequence_Check(obj.ptr())) {
      if (::PySequence_SetItem(obj.ptr(), index, CJSObject::Wrap(value).ptr()) < 0)
        isolate->ThrowException(
            v8::Exception::Error(v8::String::NewFromUtf8(isolate, "fail to set indexed value").ToLocalChecked()));
    } else if (::PyMapping_Check(obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);

      if (::PyMapping_SetItemString(obj.ptr(), buf, CJSObject::Wrap(value).ptr()) < 0)
        isolate->ThrowException(
            v8::Exception::Error(v8::String::NewFromUtf8(isolate, "fail to set named value").ToLocalChecked()));
    }

    return value;
  });

  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Integer>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    if (PyGen_Check(obj.ptr())) {
      return v8::Integer::New(isolate, v8::ReadOnly);
    }

    if (::PySequence_Check(obj.ptr())) {
      if ((Py_ssize_t)index < ::PySequence_Size(obj.ptr())) {
        return v8::Integer::New(isolate, v8::None);
      } else {
        return v8::Local<v8::Integer>();
      }
    }

    if (::PyMapping_Check(obj.ptr())) {
      // TODO: revisit this
      char buf[65];
      snprintf(buf, sizeof(buf), "%d", index);

      if (::PyMapping_HasKeyString(obj.ptr(), buf) || ::PyMapping_HasKey(obj.ptr(), py::long_(index).ptr())) {
        return v8::Integer::New(isolate, v8::None);
      } else {
        return v8::Local<v8::Integer>();
      }
    }

    return v8::Local<v8::Integer>();
  });

  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Integer>(); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());

    if (::PySequence_Check(obj.ptr()) && (Py_ssize_t)index < ::PySequence_Size(obj.ptr())) {
      auto result = 0 <= ::PySequence_DelItem(obj.ptr(), index);
      return v8::Boolean::New(isolate, result);
    }
    if (::PyMapping_Check(obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);
      auto result = PyMapping_DelItemString(obj.ptr(), buf) == 0;
      return v8::Boolean::New(isolate, result);
    }
    return v8::Local<v8::Boolean>();
  });

  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Boolean>(); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Array>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object obj = CJSObject::Wrap(info.Holder());
    Py_ssize_t len = ::PySequence_Check(obj.ptr()) ? ::PySequence_Size(obj.ptr()) : 0;
    v8::Local<v8::Array> result = v8::Array::New(isolate, len);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    for (Py_ssize_t i = 0; i < len; i++) {
      result->Set(context, v8::Integer::New(isolate, i), v8::Integer::New(isolate, i)).Check();
    }

    return result;
  });

  auto result_handle = value_or(result, [&]() { return v8::Local<v8::Array>(); });
  info.GetReturnValue().Set(result_handle);
}

#define GEN_ARG(z, n, data) CJSObject::Wrap(info[n])
#define GEN_ARGS(count) BOOST_PP_ENUM(count, GEN_ARG, NULL)

#define GEN_CASE_PRED(r, state)                                                                        \
  BOOST_PP_NOT_EQUAL(BOOST_PP_TUPLE_ELEM(2, 0, state), BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 1, state))) \
  /**/

#define GEN_CASE_OP(r, state) (BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, state)), BOOST_PP_TUPLE_ELEM(2, 1, state)) /**/

#define GEN_CASE_MACRO(r, state)                               \
  case BOOST_PP_TUPLE_ELEM(2, 0, state): {                     \
    result = self(GEN_ARGS(BOOST_PP_TUPLE_ELEM(2, 0, state))); \
    break;                                                     \
  }                                                            \
    /**/

void CPythonObject::Caller(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  if (v8u::executionTerminating(isolate)) {
    info.GetReturnValue().Set(v8::Undefined(isolate));
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(isolate, [&]() {
    CPythonGIL python_gil;

    py::object self;

    if (!info.Data().IsEmpty() && info.Data()->IsExternal()) {
      v8::Local<v8::External> field = v8::Local<v8::External>::Cast(info.Data());

      self = *static_cast<py::object*>(field->Value());
    } else {
      self = CJSObject::Wrap(info.This());
    }

    py::object result;

    switch (info.Length()) {
      BOOST_PP_FOR((0, 10), GEN_CASE_PRED, GEN_CASE_OP, GEN_CASE_MACRO)
      default:
        isolate->ThrowException(
            v8::Exception::Error(v8::String::NewFromUtf8(isolate, "too many arguments").ToLocalChecked()));

        return v8::Undefined(isolate).As<v8::Value>();
    }

    return Wrap(result);
  });

  auto result_handle = value_or(result, [&]() { return v8::Undefined(isolate); });
  info.GetReturnValue().Set(result_handle);
}

void CPythonObject::SetupObjectTemplate(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> clazz) {
  v8::HandleScope handle_scope(isolate);

  clazz->SetInternalFieldCount(1);
  clazz->SetHandler(
      v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator));
  clazz->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter, IndexedEnumerator);
  clazz->SetCallAsFunctionHandler(Caller);
}

v8::Local<v8::ObjectTemplate> CPythonObject::CreateObjectTemplate(v8::Isolate* isolate) {
  v8::EscapableHandleScope handle_scope(isolate);

  v8::Local<v8::ObjectTemplate> clazz = v8::ObjectTemplate::New(isolate);

  SetupObjectTemplate(isolate, clazz);

  return handle_scope.Escape(clazz);
}

v8::Local<v8::ObjectTemplate> CPythonObject::GetCachedObjectTemplateOrCreate(v8::Isolate* isolate) {
  v8::EscapableHandleScope handle_scope(isolate);
  // retrieve cached object template from the isolate
  auto template_ptr = isolate->GetData(kJSObjectTemplate);
  if (!template_ptr) {
    // it hasn't been created yet = > go create it and cache it in the isolate
    auto template_val = CreateObjectTemplate(isolate);
    auto eternal_val_ptr = new v8::Eternal<v8::ObjectTemplate>(isolate, template_val);
    assert(isolate->GetNumberOfDataSlots() > kJSObjectTemplate);
    isolate->SetData(kJSObjectTemplate, eternal_val_ptr);
    template_ptr = eternal_val_ptr;
  }
  // convert raw pointer to pointer to eternal handle
  auto template_eternal_val = static_cast<v8::Eternal<v8::ObjectTemplate>*>(template_ptr);
  assert(template_eternal_val);
  // retrieve local handle from eternal handle
  auto template_val = template_eternal_val->Get(isolate);
  return handle_scope.Escape(template_val);
}

bool CPythonObject::IsWrapped(v8::Local<v8::Object> obj) {
  return obj->InternalFieldCount() == 1;
}

py::object CPythonObject::Unwrap(v8::Local<v8::Object> obj) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::External> payload = v8::Local<v8::External>::Cast(obj->GetInternalField(0));

  return *static_cast<py::object*>(payload->Value());
}

void CPythonObject::Dispose(v8::Local<v8::Value> value) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  if (value->IsObject()) {
    v8::MaybeLocal<v8::Object> objMaybe = value->ToObject(isolate->GetCurrentContext());

    if (objMaybe.IsEmpty()) {
      return;
    }

    v8::Local<v8::Object> obj = objMaybe.ToLocalChecked();

    if (IsWrapped(obj)) {
      Py_DECREF(CPythonObject::Unwrap(obj).ptr());
    }
  }
}

v8::Local<v8::Value> CPythonObject::Wrap(py::object obj) {
  v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::Value> value;

  value = ObjectTracer::FindCache(obj);

  if (value.IsEmpty())

    value = WrapInternal(obj);

  return handle_scope.Escape(value);
}

v8::Local<v8::Value> CPythonObject::WrapInternal(py::object obj) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  assert(isolate->InContext());

  v8::EscapableHandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);

  CPythonGIL python_gil;

  if (v8u::executionTerminating(isolate)) {
    return v8::Undefined(isolate);
  }

  if (obj.is_none())
    return v8::Null(isolate);
  if (obj.ptr() == Py_True)
    return v8::True(isolate);
  if (obj.ptr() == Py_False)
    return v8::False(isolate);

  py::extract<CJSObject&> extractor(obj);

  if (extractor.check()) {
    CJSObject& jsobj = extractor();

    if (dynamic_cast<CJavascriptNull*>(&jsobj))
      return v8::Null(isolate);
    if (dynamic_cast<CJavascriptUndefined*>(&jsobj))
      return v8::Undefined(isolate);

    if (jsobj.Object().IsEmpty()) {
      ILazyObject* pLazyObject = dynamic_cast<ILazyObject*>(&jsobj);

      if (pLazyObject)
        pLazyObject->LazyConstructor();
    }

    if (jsobj.Object().IsEmpty()) {
      throw CJavascriptException("Refer to a null object", ::PyExc_AttributeError);
    }

    py::object* object = new py::object(obj);

    ObjectTracer::Trace(jsobj.Object(), object);

    return handle_scope.Escape(jsobj.Object());
  }

  v8::Local<v8::Value> result;

  if (PyLong_CheckExact(obj.ptr())) {
    result = v8::Integer::New(isolate, ::PyLong_AsLong(obj.ptr()));
  } else if (PyBool_Check(obj.ptr())) {
    result = v8::Boolean::New(isolate, py::extract<bool>(obj));
  } else if (PyBytes_CheckExact(obj.ptr()) || PyUnicode_CheckExact(obj.ptr())) {
    result = v8u::toString(obj);
  } else if (PyFloat_CheckExact(obj.ptr())) {
    result = v8::Number::New(isolate, py::extract<double>(obj));
  } else if (isExactDateTime(obj) || isExactDate(obj)) {
    tm ts = {0};
    int ms = 0;
    getPythonDateTime(obj, ts, ms);
    result = v8::Date::New(isolate->GetCurrentContext(), ((double)mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
  } else if (isExactTime(obj)) {
    tm ts = {0};
    int ms = 0;
    getPythonTime(obj, ts, ms);
    result = v8::Date::New(isolate->GetCurrentContext(), ((double)mktime(&ts)) * 1000 + ms / 1000).ToLocalChecked();
  } else if (PyCFunction_Check(obj.ptr()) || PyFunction_Check(obj.ptr()) || PyMethod_Check(obj.ptr()) ||
             PyType_CheckExact(obj.ptr())) {
    v8::Local<v8::FunctionTemplate> func_tmpl = v8::FunctionTemplate::New(isolate);
    py::object* object = new py::object(obj);

    func_tmpl->SetCallHandler(Caller, v8::External::New(isolate, object));

    if (PyType_Check(obj.ptr())) {
      v8::Local<v8::String> cls_name =
          v8::String::NewFromUtf8(isolate, py::extract<const char*>(obj.attr("__name__"))()).ToLocalChecked();

      func_tmpl->SetClassName(cls_name);
    }

    result = func_tmpl->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();

    if (!result.IsEmpty())
      ObjectTracer::Trace(result, object);
  } else {
    auto template_val = GetCachedObjectTemplateOrCreate(isolate);
    auto instance = template_val->NewInstance(isolate->GetCurrentContext());
    if (!instance.IsEmpty()) {
      py::object* object = new py::object(obj);

      v8::Local<v8::Object> realInstance = instance.ToLocalChecked();
      realInstance->SetInternalField(0, v8::External::New(isolate, object));

      ObjectTracer::Trace(instance.ToLocalChecked(), object);

      result = realInstance;
    }
  }

  if (result.IsEmpty())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return handle_scope.Escape(result);
}