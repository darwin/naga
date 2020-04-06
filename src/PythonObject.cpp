#include "PythonObject.h"
#include "JSObject.h"
#include "JSObjectNull.h"
#include "JSObjectUndefined.h"
#include "Isolate.h"
#include "PythonDateTime.h"
#include "Tracer.h"
#include "PythonGIL.h"
#include "PythonExceptionGuard.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

// static std::ostream& operator<<(std::ostream& os, const CJSObject& obj) {
//  obj.Dump(os);
//
//  return os;
//}

static std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj) {
  obj->Dump(os);
  return os;
}

void CPythonObject::ThrowIf(v8::Isolate* v8_isolate, const py::error_already_set& e) {
  CPythonGIL python_gil;

  auto v8_scope = v8u::getScope(v8_isolate);

  py::object py_type(e.type());
  py::object py_value(e.value());

  PyObject* raw_val = py_value.ptr();

  //  TODO: investigate: shall we call normalize, pybind does not call it
  //  PyErr_NormalizeException(&raw_exc, &raw_val, &raw_trb);

  std::string msg;

  if (py::hasattr(py_value, "args")) {
    auto py_args = py_value.attr("args");
    if (py::isinstance<py::tuple>(py_args)) {
      auto py_args_tuple = py::cast<py::tuple>(py_args);
      auto it = py_args_tuple.begin();
      while (it != py_args_tuple.end()) {
        auto py_arg = *it;
        if (py::isinstance<py::str>(py_arg)) {
          msg += py::cast<py::str>(py_arg);
        }
        it++;
      }
    }
  } else if (py::hasattr(py_value, "message")) {
    auto py_msg = py_value.attr("message");
    if (py::isinstance<py::str>(py_msg)) {
      msg += py::cast<py::str>(py_msg);
    }
  } else if (raw_val) {
    // TODO: use pybind
    if (PyBytes_CheckExact(raw_val)) {
      msg = PyBytes_AS_STRING(raw_val);
    } else if (PyTuple_CheckExact(raw_val)) {
      for (int i = 0; i < PyTuple_GET_SIZE(raw_val); i++) {
        PyObject* raw_item = PyTuple_GET_ITEM(raw_val, i);

        if (raw_item && PyBytes_CheckExact(raw_item)) {
          msg = PyBytes_AS_STRING(raw_item);
          break;
        }
      }
    }
  }

  v8::Local<v8::Value> v8_error;

  if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_IndexError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::RangeError(v8_msg);
  } else if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_AttributeError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::ReferenceError(v8_msg);
  } else if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_SyntaxError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::SyntaxError(v8_msg);
  } else if (PyErr_GivenExceptionMatches(py_type.ptr(), PyExc_TypeError)) {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::TypeError(v8_msg);
  } else {
    auto v8_msg =
        v8::String::NewFromUtf8(v8_isolate, msg.c_str(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    v8_error = v8::Exception::Error(v8_msg);
  }

  if (v8_error->IsObject()) {
    auto v8_context = v8_isolate->GetCurrentContext();

    auto v8_exc_type_key = v8::String::NewFromUtf8(v8_isolate, "exc_type").ToLocalChecked();
    auto v8_exc_value_key = v8::String::NewFromUtf8(v8_isolate, "exc_value").ToLocalChecked();

    auto v8_exc_type_api = v8::Private::ForApi(v8_isolate, v8_exc_type_key);
    auto v8_exc_value_api = v8::Private::ForApi(v8_isolate, v8_exc_value_key);

    // TODO: get rid of manual refcounting

    Py_INCREF(py_type.ptr());
    Py_INCREF(py_value.ptr());

    auto v8_exc_type_external = v8::External::New(v8_isolate, py_type.ptr());
    auto v8_exc_value_external = v8::External::New(v8_isolate, py_value.ptr());

    v8_error->ToObject(v8_context).ToLocalChecked()->SetPrivate(v8_context, v8_exc_type_api, v8_exc_type_external);
    v8_error->ToObject(v8_context).ToLocalChecked()->SetPrivate(v8_context, v8_exc_value_api, v8_exc_value_external);
  }

  v8_isolate->ThrowException(v8_error);
}

void CPythonObject::NamedGetter(v8::Local<v8::Name> v8_prop_name, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    auto v8_utf_name = v8u::toUtf8Value(v8_isolate, v8_prop_name.As<v8::String>());

    // TODO: use pybind
    if (PyGen_Check(py_obj.ptr())) {
      return v8::Undefined(v8_isolate).As<v8::Value>();
    }

    if (!*v8_utf_name) {
      return v8::Undefined(v8_isolate).As<v8::Value>();
    }

    py::object py_val;
    try {
      py_val = py::getattr(py_obj, *v8_utf_name);
    } catch (const py::error_already_set& e) {
      if (e.matches(PyExc_AttributeError)) {
        // TODO: revisit this, what is the difference between mapping and hasattr?
        if (PyMapping_Check(py_obj.ptr()) && PyMapping_HasKeyString(py_obj.ptr(), *v8_utf_name)) {
          auto raw_item = PyMapping_GetItemString(py_obj.ptr(), *v8_utf_name);
          auto py_result(py::reinterpret_steal<py::object>(raw_item));
          return Wrap(py_result);
        }
        return v8::Local<v8::Value>();
      } else {
        throw e;
      }
    }

    if (PyObject_TypeCheck(py_val.ptr(), &PyProperty_Type)) {
      auto getter = py_val.attr("fget");

      if (getter.is_none()) {
        throw CJSException("unreadable attribute", PyExc_AttributeError);
      }

      py_val = getter();
    }

    return Wrap(py_val);
  });

  auto result_handle = value_or(result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(result_handle);
}

void CPythonObject::NamedSetter(v8::Local<v8::Name> v8_prop_name,
                                v8::Local<v8::Value> v8_value,
                                const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    v8::String::Utf8Value v8_utf_name(v8_isolate, v8_prop_name);
    auto py_val = CJSObject::Wrap(v8_value);
    bool found = py::hasattr(py_obj, *v8_utf_name);

    // TODO: review this after learning about __watchpoints__
    if (py::hasattr(py_obj, "__watchpoints__")) {
      py::dict py_watch_points(py_obj.attr("__watchpoints__"));
      py::str py_prop_name(*v8_utf_name, v8_utf_name.length());
      if (py_watch_points.contains(py_prop_name)) {
        auto py_watch_handler = py_watch_points[py_prop_name];
        auto py_attr = found ? py::object(py_obj.attr(*v8_utf_name)) : py::none();
        py_val = py_watch_handler(py_prop_name, py_attr, py_val);
      }
    }

    // TODO: revisit
    if (!found && PyMapping_Check(py_obj.ptr())) {
      PyMapping_SetItemString(py_obj.ptr(), *v8_utf_name, py_val.ptr());
    } else {
      if (found) {
        auto py_name_attr = py_obj.attr(*v8_utf_name);

        if (PyObject_TypeCheck(py_name_attr.ptr(), &PyProperty_Type)) {
          auto setter = py_name_attr.attr("fset");

          if (setter.is_none()) {
            throw CJSException("can't set attribute", PyExc_AttributeError);
          }

          setter(py_val);
          return v8_value;
        }
      }

      py_obj.attr(*v8_utf_name) = py_val;
    }

    return v8_value;
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::NamedQuery(v8::Local<v8::Name> v8_prop_name, const v8::PropertyCallbackInfo<v8::Integer>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Integer>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    v8::String::Utf8Value name(v8_isolate, v8_prop_name);

    // TODO: rewrite using pybind
    bool exists = PyGen_Check(py_obj.ptr()) || PyObject_HasAttrString(py_obj.ptr(), *name) ||
                  (PyMapping_Check(py_obj.ptr()) && PyMapping_HasKeyString(py_obj.ptr(), *name));

    if (exists) {
      return v8::Integer::New(v8_isolate, v8::None);
    } else {
      return v8::Local<v8::Integer>();
    }
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Integer>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::NamedDeleter(v8::Local<v8::Name> v8_prop_name,
                                 const v8::PropertyCallbackInfo<v8::Boolean>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_prop_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }
  v8::HandleScope handle_scope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    v8::String::Utf8Value name(v8_isolate, v8_prop_name);

    // TODO: rewrite using pybind
    if (!PyObject_HasAttrString(py_obj.ptr(), *name) && PyMapping_Check(py_obj.ptr()) &&
        PyMapping_HasKeyString(py_obj.ptr(), *name)) {
      return v8::Boolean::New(v8_isolate, -1 != PyMapping_DelItemString(py_obj.ptr(), *name));
    } else {
      auto py_name_attr = py_obj.attr(*name);

      if (PyObject_HasAttrString(py_obj.ptr(), *name) && PyObject_TypeCheck(py_name_attr.ptr(), &PyProperty_Type)) {
        auto py_deleter = py_name_attr.attr("fdel");

        if (py_deleter.is_none()) {
          throw CJSException("can't delete attribute", PyExc_AttributeError);
        }
        auto py_result = py_deleter();
        auto py_bool_result = py::cast<py::bool_>(py_result);
        return v8::Boolean::New(v8_isolate, py_bool_result);
      } else {
        auto result = -1 != PyObject_DelAttrString(py_obj.ptr(), *name);
        return v8::Boolean::New(v8_isolate, result);
      }
    }
  });

  auto result_handle = value_or(v8_result, [&]() { return v8::Local<v8::Boolean>(); });
  v8_info.GetReturnValue().Set(result_handle);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-lambda-function-name"
void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  v8::HandleScope handle_scope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Array>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    py::list keys;
    bool filter_name = false;

    if (PySequence_Check(py_obj.ptr())) {
      return v8::Local<v8::Array>();
    } else if (PyMapping_Check(py_obj.ptr())) {
      keys = py::reinterpret_steal<py::list>(PyMapping_Keys(py_obj.ptr()));
    } else if (PyGen_CheckExact(py_obj.ptr())) {
      auto py_iter(py::reinterpret_steal<py::object>(PyObject_GetIter(py_obj.ptr())));

      PyObject* raw_item = nullptr;

      while (nullptr != (raw_item = PyIter_Next(py_iter.ptr()))) {
        keys.append(py::reinterpret_steal<py::object>(raw_item));
      }
    } else {
      keys = py::reinterpret_steal<py::list>(PyObject_Dir(py_obj.ptr()));
      filter_name = true;
    }

    Py_ssize_t len = PyList_GET_SIZE(keys.ptr());
    v8::Local<v8::Array> v8_array = v8::Array::New(v8_isolate, len);
    for (Py_ssize_t i = 0; i < len; i++) {
      PyObject* raw_item = PyList_GET_ITEM(keys.ptr(), i);

      if (filter_name && PyBytes_CheckExact(raw_item)) {
        std::string name(py::reinterpret_borrow<py::str>(raw_item));

        // FIXME: Are there any methods to avoid such a dirty work?
        if (name.find("__", 0) == 0 && name.rfind("__", name.size() - 2)) {
          continue;
        }
      }

      auto py_item = Wrap(py::reinterpret_borrow<py::object>(raw_item));
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      auto res = v8_array->Set(v8::Isolate::GetCurrent()->GetCurrentContext(), v8_i, py_item);
      res.Check();
    }
    return v8_array;
  });

  auto result_handle = value_or(v8_result, [&]() { return v8::Local<v8::Array>(); });
  v8_info.GetReturnValue().Set(result_handle);
}
#pragma clang diagnostic pop

void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PyGen_Check(py_obj.ptr())) {
      return v8::Undefined(v8_isolate).As<v8::Value>();
    }

    if (PySequence_Check(py_obj.ptr())) {
      if (static_cast<Py_ssize_t>(index) < PySequence_Size(py_obj.ptr())) {
        auto ret(py::reinterpret_steal<py::object>(PySequence_GetItem(py_obj.ptr(), index)));
        return Wrap(ret);
      } else {
        return v8::Undefined(v8_isolate).As<v8::Value>();
      }
    }

    if (PyMapping_Check(py_obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);

      PyObject* raw_value = PyMapping_GetItemString(py_obj.ptr(), buf);

      if (!raw_value) {
        py::int_ py_index(index);
        raw_value = PyObject_GetItem(py_obj.ptr(), py_index.ptr());
      }

      if (raw_value) {
        return Wrap(py::reinterpret_steal<py::object>(raw_value));
      } else {
        return v8::Undefined(v8_isolate).As<v8::Value>();
      }
    }

    return v8::Undefined(v8_isolate).As<v8::Value>();
  });

  auto result_handle = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(result_handle);
}

void CPythonObject::IndexedSetter(uint32_t index,
                                  v8::Local<v8::Value> v8_value,
                                  const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PySequence_Check(py_obj.ptr())) {
      if (PySequence_SetItem(py_obj.ptr(), index, CJSObject::Wrap(v8_value).ptr()) < 0) {
        auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "fail to set indexed value").ToLocalChecked();
        auto v8_ex = v8::Exception::Error(v8_msg);
        v8_isolate->ThrowException(v8_ex);
      }
    } else if (PyMapping_Check(py_obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);

      if (PyMapping_SetItemString(py_obj.ptr(), buf, CJSObject::Wrap(v8_value).ptr()) < 0) {
        auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "fail to set named value").ToLocalChecked();
        auto v8_ex = v8::Exception::Error(v8_msg);
        v8_isolate->ThrowException(v8_ex);
      }
    }

    return v8_value;
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::IndexedQuery(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Integer>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PyGen_Check(py_obj.ptr())) {
      return v8::Integer::New(v8_isolate, v8::ReadOnly);
    }

    if (PySequence_Check(py_obj.ptr())) {
      if (static_cast<Py_ssize_t>(index) < PySequence_Size(py_obj.ptr())) {
        return v8::Integer::New(v8_isolate, v8::None);
      } else {
        return v8::Local<v8::Integer>();
      }
    }

    if (PyMapping_Check(py_obj.ptr())) {
      // TODO: revisit this
      char buf[65];
      snprintf(buf, sizeof(buf), "%d", index);

      auto py_index = py::int_(index);
      if (PyMapping_HasKeyString(py_obj.ptr(), buf) || PyMapping_HasKey(py_obj.ptr(), py_index.ptr())) {
        return v8::Integer::New(v8_isolate, v8::None);
      } else {
        return v8::Local<v8::Integer>();
      }
    }

    return v8::Local<v8::Integer>();
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Integer>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::IndexedDeleter(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());

    if (PySequence_Check(py_obj.ptr()) && static_cast<Py_ssize_t>(index) < PySequence_Size(py_obj.ptr())) {
      auto result = 0 <= PySequence_DelItem(py_obj.ptr(), index);
      return v8::Boolean::New(v8_isolate, result);
    }
    if (PyMapping_Check(py_obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);
      auto result = PyMapping_DelItemString(py_obj.ptr(), buf) == 0;
      return v8::Boolean::New(v8_isolate, result);
    }
    return v8::Local<v8::Boolean>();
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Boolean>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::IndexedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Array>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    Py_ssize_t len = PySequence_Check(py_obj.ptr()) ? PySequence_Size(py_obj.ptr()) : 0;
    auto v8_array = v8::Array::New(v8_isolate, len);
    auto v8_context = v8_isolate->GetCurrentContext();

    for (Py_ssize_t i = 0; i < len; i++) {
      auto v8_i = v8::Integer::New(v8_isolate, i);
      v8_array->Set(v8_context, v8_i, v8_i).Check();
    }
    return v8_array;
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Array>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::Caller(const v8::FunctionCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::getScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    CPythonGIL python_gil;

    py::object py_self;

    if (!v8_info.Data().IsEmpty() && v8_info.Data()->IsExternal()) {
      auto v8_field = v8_info.Data().As<v8::External>();
      auto raw_self = static_cast<PyObject*>(v8_field->Value());
      py_self = py::reinterpret_borrow<py::object>(raw_self);
    } else {
      py_self = CJSObject::Wrap(v8_info.This());
    }

    py::object py_result;

    switch (v8_info.Length()) {
      // clang-format off
      case 0: {
        py_result = py_self();
        break;
      }
      case 1: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]));
        break;
      }
      case 2: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]));
        break;
      }
      case 3: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]));
        break;
      }
      case 4: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]));
        break;
      }
      case 5: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]));
        break;
      }
      case 6: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]));
        break;
      }
      case 7: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]));
        break;
      }
      case 8: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]),
                      CJSObject::Wrap(v8_info[7]));
        break;
      }
      case 9: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]),
                      CJSObject::Wrap(v8_info[7]),
                      CJSObject::Wrap(v8_info[8]));
        break;
      }
      case 10: {
        py_result = py_self(CJSObject::Wrap(v8_info[0]),
                      CJSObject::Wrap(v8_info[1]),
                      CJSObject::Wrap(v8_info[2]),
                      CJSObject::Wrap(v8_info[3]),
                      CJSObject::Wrap(v8_info[4]),
                      CJSObject::Wrap(v8_info[5]),
                      CJSObject::Wrap(v8_info[6]),
                      CJSObject::Wrap(v8_info[7]),
                      CJSObject::Wrap(v8_info[8]),
                      CJSObject::Wrap(v8_info[9]));
        break;
      }
        // clang-format on
      default: {
        auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "too many arguments").ToLocalChecked();
        v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
        return v8::Undefined(v8_isolate).As<v8::Value>();
      }
    }

    return Wrap(py_result);
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
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

bool CPythonObject::IsWrapped2(v8::Local<v8::Object> v8_obj) {
  return v8_obj->InternalFieldCount() > 0;
}

py::object CPythonObject::GetWrapper2(v8::Local<v8::Object> v8_obj) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_val = v8_obj->GetInternalField(0);
  assert(!v8_val.IsEmpty());
  auto v8_payload = v8_val.As<v8::External>();
  auto raw_obj = static_cast<PyObject*>(v8_payload->Value());
  return py::reinterpret_borrow<py::object>(raw_obj);
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

    // TODO: revisit this
    if (IsWrapped2(obj)) {
      Py_DECREF(CPythonObject::GetWrapper2(obj).ptr());
    }
  }
}

v8::Local<v8::Value> CPythonObject::Wrap(py::handle py_obj) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::openEscapableScope(v8_isolate);

  auto value = ObjectTracer::FindCache(py_obj.ptr());
  if (value.IsEmpty()) {
    value = WrapInternal2(py_obj);
  }
  return v8_scope.Escape(value);
}

v8::Local<v8::Value> CPythonObject::WrapInternal2(py::handle py_obj) {
  auto v8_isolate = v8::Isolate::GetCurrent();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::openEscapableScope(v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  CPythonGIL python_gil;

  if (v8u::executionTerminating(v8_isolate)) {
    return v8::Undefined(v8_isolate);
  }

  if (py_obj.is_none()) {
    return v8::Null(v8_isolate);
  }
  if (py::isinstance<py::bool_>(py_obj)) {
    auto py_bool = py::cast<py::bool_>(py_obj);
    if (py_bool) {
      return v8::True(v8_isolate);
    } else {
      return v8::False(v8_isolate);
    }
  }

  if (py::isinstance<CJSObjectNull>(py_obj)) {
    return v8::Null(v8_isolate);
  }

  if (py::isinstance<CJSObjectUndefined>(py_obj)) {
    return v8::Undefined(v8_isolate);
  }

  if (py::isinstance<CJSObject>(py_obj)) {
    auto obj = py::cast<CJSObjectPtr>(py_obj);
    assert(obj.get());
    obj->LazyInit();

    if (obj->Object().IsEmpty()) {
      throw CJSException("Refer to a null object", PyExc_AttributeError);
    }

    // ObjectTracer2::Trace(obj->Object(), py_obj.ptr());
    return v8_scope.Escape(obj->Object());
  }

  v8::Local<v8::Value> v8_result;

  // TODO: replace this with pybind code
  if (PyLong_CheckExact(py_obj.ptr())) {
    v8_result = v8::Integer::New(v8_isolate, PyLong_AsLong(py_obj.ptr()));
  } else if (PyBool_Check(py_obj.ptr())) {
    v8_result = v8::Boolean::New(v8_isolate, py::cast<py::bool_>(py_obj));
  } else if (PyBytes_CheckExact(py_obj.ptr()) || PyUnicode_CheckExact(py_obj.ptr())) {
    v8_result = v8u::toString(py_obj);
  } else if (PyFloat_CheckExact(py_obj.ptr())) {
    v8_result = v8::Number::New(v8_isolate, py::cast<py::float_>(py_obj));
  } else if (isExactDateTime(py_obj) || isExactDate(py_obj)) {
    tm ts = {0};
    int ms = 0;
    getPythonDateTime(py_obj, ts, ms);
    v8_result = v8::Date::New(v8_isolate->GetCurrentContext(), (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000)
                    .ToLocalChecked();
  } else if (isExactTime(py_obj)) {
    tm ts = {0};
    int ms = 0;
    getPythonTime(py_obj, ts, ms);
    v8_result = v8::Date::New(v8_isolate->GetCurrentContext(), (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000)
                    .ToLocalChecked();
  } else if (PyCFunction_Check(py_obj.ptr()) || PyFunction_Check(py_obj.ptr()) || PyMethod_Check(py_obj.ptr()) ||
             PyType_CheckExact(py_obj.ptr())) {
    auto func_tmpl = v8::FunctionTemplate::New(v8_isolate);

    // NOTE: tracker will keep the object alive
    ObjectTracer::Trace(v8_result, py_obj.ptr());
    // Py_INCREF(py_obj.ptr());
    func_tmpl->SetCallHandler(Caller, v8::External::New(v8_isolate, py_obj.ptr()));

    if (PyType_Check(py_obj.ptr())) {
      auto py_name_attr = py_obj.attr("__name__");
      // TODO: we should do it safer here, if __name__ is not string
      auto v8_cls_name = v8u::toString(py_name_attr);
      func_tmpl->SetClassName(v8_cls_name);
    }
    v8_result = func_tmpl->GetFunction(v8_isolate->GetCurrentContext()).ToLocalChecked();
    //    if (!v8_result.IsEmpty()) {
    //      ObjectTracer2::Trace(v8_result, py_obj.ptr());
    //    }
  } else {
    auto template_val = GetCachedObjectTemplateOrCreate(v8_isolate);
    auto instance = template_val->NewInstance(v8_isolate->GetCurrentContext());
    if (!instance.IsEmpty()) {
      auto realInstance = instance.ToLocalChecked();
      // NOTE: tracker will keep the object alive
      ObjectTracer::Trace(instance.ToLocalChecked(), py_obj.ptr());
      // Py_INCREF(py_obj.ptr());
      realInstance->SetInternalField(0, v8::External::New(v8_isolate, py_obj.ptr()));
      v8_result = realInstance;
    }
  }

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return v8_scope.Escape(v8_result);
}

#pragma clang diagnostic pop