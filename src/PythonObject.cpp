#include "PythonObject.h"
#include "JSObject.h"
#include "JSObjectNull.h"
#include "JSObjectUndefined.h"
#include "Isolate.h"
#include "PythonDateTime.h"
#include "Tracer.h"
#include "PythonExceptionGuard.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#define TRACE(...) (SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__))

void CPythonObject::ThrowIf(const v8::IsolateRef& v8_isolate, const py::error_already_set& py_ex) {
  TRACE("CPythonObject::ThrowIf");
  auto py_gil = pyu::acquireGIL();

  auto v8_scope = v8u::openScope(v8_isolate);

  py::object py_type(py_ex.type());
  py::object py_value(py_ex.value());

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
        auto raw_item = PyTuple_GET_ITEM(raw_val, i);

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

void CPythonObject::NamedGetter(v8::Local<v8::Name> v8_name, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::NamedGetter v8_name={} v8_info={}", v8_name, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    auto v8_utf_name = v8u::toUtf8Value(v8_isolate, v8_name.As<v8::String>());

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

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::NamedSetter(v8::Local<v8::Name> v8_name,
                                v8::Local<v8::Value> v8_value,
                                const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::NamedSetter v8_name={} v8_value={} v8_info={}", v8_name, v8_value, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    v8::String::Utf8Value v8_utf_name(v8_isolate, v8_name);
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

void CPythonObject::NamedQuery(v8::Local<v8::Name> v8_name, const v8::PropertyCallbackInfo<v8::Integer>& v8_info) {
  TRACE("CPythonObject::NamedQuery v8_name={} v8_info={}", v8_name, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Integer>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    v8::String::Utf8Value name(v8_isolate, v8_name);

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

void CPythonObject::NamedDeleter(v8::Local<v8::Name> v8_name, const v8::PropertyCallbackInfo<v8::Boolean>& v8_info) {
  TRACE("CPythonObject::NamedQuery v8_name={} v8_info={}", v8_name, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_name->IsSymbol()) {
    // ignore symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    v8::String::Utf8Value name(v8_isolate, v8_name);

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

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Boolean>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-lambda-function-name"
void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info) {
  TRACE("CPythonObject::NamedEnumerator v8_info={}", v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Array>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
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

    auto len = PyList_GET_SIZE(keys.ptr());
    auto v8_array = v8::Array::New(v8_isolate, len);
    for (Py_ssize_t i = 0; i < len; i++) {
      auto raw_item = PyList_GET_ITEM(keys.ptr(), i);

      if (filter_name && PyBytes_CheckExact(raw_item)) {
        std::string name(py::reinterpret_borrow<py::str>(raw_item));

        // FIXME: Are there any methods to avoid such a dirty work?
        if (name.find("__", 0) == 0 && name.rfind("__", name.size() - 2)) {
          continue;
        }
      }

      auto py_item = Wrap(py::reinterpret_borrow<py::object>(raw_item));
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      auto res = v8_array->Set(v8u::getCurrentIsolate()->GetCurrentContext(), v8_i, py_item);
      res.Check();
    }
    return v8_array;
  });

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Local<v8::Array>(); });
  v8_info.GetReturnValue().Set(v8_final_result);
}
#pragma clang diagnostic pop

void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::IndexedGetter index={} v8_info={}", index, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
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

  auto v8_final_result = value_or(v8_result, [&]() { return v8::Undefined(v8_isolate); });
  v8_info.GetReturnValue().Set(v8_final_result);
}

void CPythonObject::IndexedSetter(uint32_t index,
                                  v8::Local<v8::Value> v8_value,
                                  const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::IndexedSetter index={} v8_value={} v8_info={}", index, v8_value, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
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
  TRACE("CPythonObject::IndexedQuery index={} v8_info={}", index, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Integer>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
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
  TRACE("CPythonObject::IndexedDeleter index={} v8_info={}", index, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Boolean>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
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
  TRACE("CPythonObject::IndexedEnumerator v8_info={}", v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Array>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
    auto py_obj = CJSObject::Wrap(v8_info.Holder());
    auto len = PySequence_Check(py_obj.ptr()) ? PySequence_Size(py_obj.ptr()) : 0;
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
  TRACE("CPythonObject::Caller v8_info={}", v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard<v8::Local<v8::Value>>(v8_isolate, [&]() {
    auto py_gil = pyu::acquireGIL();
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

void CPythonObject::SetupObjectTemplate(const v8::IsolateRef& v8_isolate,
                                        v8::Local<v8::ObjectTemplate> v8_object_template) {
  TRACE("CPythonObject::SetupObjectTemplate");
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_handler_config =
      v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator);

  v8_object_template->SetInternalFieldCount(1);
  v8_object_template->SetHandler(v8_handler_config);
  v8_object_template->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter,
                                                IndexedEnumerator);
  v8_object_template->SetCallAsFunctionHandler(Caller);
}

v8::Local<v8::ObjectTemplate> CPythonObject::CreateObjectTemplate(const v8::IsolateRef& v8_isolate) {
  TRACE("CPythonObject::CreateObjectTemplate");
  auto v8_scope = v8u::openEscapableScope(v8_isolate);
  auto v8_class = v8::ObjectTemplate::New(v8_isolate);

  SetupObjectTemplate(v8_isolate, v8_class);

  return v8_scope.Escape(v8_class);
}

v8::Local<v8::ObjectTemplate> CPythonObject::GetCachedObjectTemplateOrCreate(const v8::IsolateRef& v8_isolate) {
  TRACE("CPythonObject::GetCachedObjectTemplateOrCreate");
  auto v8_scope = v8u::openEscapableScope(v8_isolate);
  // retrieve cached object template from the isolate
  auto template_ptr = v8_isolate->GetData(kJSObjectTemplate);
  if (!template_ptr) {
    TRACE("  => creating template");
    // it hasn't been created yet = > go create it and cache it in the isolate
    auto v8_born_template = CreateObjectTemplate(v8_isolate);
    auto v8_eternal_born_template = new v8::Eternal<v8::ObjectTemplate>(v8_isolate, v8_born_template);
    assert(v8_isolate->GetNumberOfDataSlots() > kJSObjectTemplate);
    v8_isolate->SetData(kJSObjectTemplate, v8_eternal_born_template);
    template_ptr = v8_eternal_born_template;
  }
  // convert raw pointer to pointer to eternal handle
  auto v8_eternal_val = static_cast<v8::Eternal<v8::ObjectTemplate>*>(template_ptr);
  assert(v8_eternal_val);
  // retrieve local handle from eternal handle
  auto v8_template = v8_eternal_val->Get(v8_isolate);
  return v8_scope.Escape(v8_template);
}

bool CPythonObject::IsWrapped(v8::Local<v8::Object> v8_object) {
  TRACE("CPythonObject::IsWrapped v8_object={}", v8_object);
  return v8_object->InternalFieldCount() > 0;
}

py::object CPythonObject::GetWrapper(v8::Local<v8::Object> v8_object) {
  TRACE("CPythonObject::GetWrapper v8_object={}", v8_object);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_val = v8_object->GetInternalField(0);
  assert(!v8_val.IsEmpty());
  auto v8_payload = v8_val.As<v8::External>();
  auto raw_obj = static_cast<PyObject*>(v8_payload->Value());
  return py::reinterpret_borrow<py::object>(raw_obj);
}

void CPythonObject::Dispose(v8::Local<v8::Value> v8_value) {
  TRACE("CPythonObject::Dispose v8_value={}", v8_value);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);

  if (v8_value->IsObject()) {
    v8::MaybeLocal<v8::Object> v8_maybe_object = v8_value->ToObject(v8_isolate->GetCurrentContext());
    if (v8_maybe_object.IsEmpty()) {
      return;
    }

    auto v8_object = v8_maybe_object.ToLocalChecked();

    // TODO: revisit this
    if (IsWrapped(v8_object)) {
      Py_DECREF(CPythonObject::GetWrapper(v8_object).ptr());
    }
  }
}

v8::Local<v8::Value> CPythonObject::Wrap(py::handle py_handle) {
  TRACE("CPythonObject::Wrap py_handle={}", py_handle);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::openEscapableScope(v8_isolate);

  auto v8_value = ObjectTracer::FindCache(py_handle.ptr());
  if (v8_value.IsEmpty()) {
    v8_value = WrapInternal(py_handle);
  }
  return v8_scope.Escape(v8_value);
}

v8::Local<v8::Value> CPythonObject::WrapInternal(py::handle py_handle) {
  TRACE("CPythonObject::WrapInternal py_handle={}", py_handle);
  auto v8_isolate = v8u::getCurrentIsolate();
  assert(v8_isolate->InContext());
  auto v8_scope = v8u::openEscapableScope(v8_isolate);
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);
  auto py_gil = pyu::acquireGIL();

  if (v8u::executionTerminating(v8_isolate)) {
    return v8::Undefined(v8_isolate);
  }

  if (py_handle.is_none()) {
    return v8::Null(v8_isolate);
  }
  if (py::isinstance<py::bool_>(py_handle)) {
    auto py_bool = py::cast<py::bool_>(py_handle);
    if (py_bool) {
      return v8::True(v8_isolate);
    } else {
      return v8::False(v8_isolate);
    }
  }

  if (py::isinstance<CJSObjectNull>(py_handle)) {
    return v8::Null(v8_isolate);
  }

  if (py::isinstance<CJSObjectUndefined>(py_handle)) {
    return v8::Undefined(v8_isolate);
  }

  if (py::isinstance<CJSObject>(py_handle)) {
    auto object = py::cast<CJSObjectPtr>(py_handle);
    assert(object.get());
    object->LazyInit();

    if (object->Object().IsEmpty()) {
      throw CJSException("Refer to a null object", PyExc_AttributeError);
    }

    // ObjectTracer2::Trace(obj->Object(), py_obj.ptr());
    return v8_scope.Escape(object->Object());
  }

  v8::Local<v8::Value> v8_result;

  // TODO: replace this with pybind code
  if (PyLong_CheckExact(py_handle.ptr())) {
    v8_result = v8::Integer::New(v8_isolate, PyLong_AsLong(py_handle.ptr()));
  } else if (PyBool_Check(py_handle.ptr())) {
    v8_result = v8::Boolean::New(v8_isolate, py::cast<py::bool_>(py_handle));
  } else if (PyBytes_CheckExact(py_handle.ptr()) || PyUnicode_CheckExact(py_handle.ptr())) {
    v8_result = v8u::toString(py_handle);
  } else if (PyFloat_CheckExact(py_handle.ptr())) {
    v8_result = v8::Number::New(v8_isolate, py::cast<py::float_>(py_handle));
  } else if (isExactDateTime(py_handle) || isExactDate(py_handle)) {
    tm ts = {0};
    int ms = 0;
    getPythonDateTime(py_handle, ts, ms);
    v8_result = v8::Date::New(v8_isolate->GetCurrentContext(), (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000)
                    .ToLocalChecked();
  } else if (isExactTime(py_handle)) {
    tm ts = {0};
    int ms = 0;
    getPythonTime(py_handle, ts, ms);
    v8_result = v8::Date::New(v8_isolate->GetCurrentContext(), (static_cast<double>(mktime(&ts))) * 1000 + ms / 1000)
                    .ToLocalChecked();
  } else if (PyCFunction_Check(py_handle.ptr()) || PyFunction_Check(py_handle.ptr()) ||
             PyMethod_Check(py_handle.ptr()) || PyType_CheckExact(py_handle.ptr())) {
    auto v8_fn_template = v8::FunctionTemplate::New(v8_isolate);
    v8_fn_template->SetCallHandler(Caller, v8::External::New(v8_isolate, py_handle.ptr()));
    if (PyType_Check(py_handle.ptr())) {
      auto py_name_attr = py_handle.attr("__name__");
      // TODO: we should do it safer here, if __name__ is not string
      auto v8_cls_name = v8u::toString(py_name_attr);
      v8_fn_template->SetClassName(v8_cls_name);
    }
    v8_result = v8_fn_template->GetFunction(v8_isolate->GetCurrentContext()).ToLocalChecked();
    assert(!v8_result.IsEmpty());
    // NOTE: tracker will keep the object alive
    ObjectTracer::Trace(v8_result, py_handle.ptr());
  } else {
    auto v8_object_template = GetCachedObjectTemplateOrCreate(v8_isolate);
    auto v8_object_instance = v8_object_template->NewInstance(v8_isolate->GetCurrentContext()).ToLocalChecked();
    ;
    assert(!v8_object_instance.IsEmpty());
    v8_object_instance->SetInternalField(0, v8::External::New(v8_isolate, py_handle.ptr()));
    // NOTE: tracker will keep the object alive
    ObjectTracer::Trace(v8_object_instance, py_handle.ptr());
    v8_result = v8_object_instance;
  }

  if (v8_result.IsEmpty()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return v8_scope.Escape(v8_result);
}

#pragma clang diagnostic pop