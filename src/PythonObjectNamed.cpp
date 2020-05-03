#include "PythonObject.h"
#include "JSException.h"
#include "PythonExceptions.h"
#include "Tracer.h"
#include "Wrapping.h"
#include "Logging.h"
#include "Printing.h"
#include "PythonUtils.h"
#include "Utils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

// TODO: this could be probably replaced with a single PyCallable_Check
bool isPythonCallable(PyObject* raw_object) {
  return PyCFunction_Check(raw_object) || PyFunction_Check(raw_object) || PyMethod_Check(raw_object) ||
         PyType_CheckExact(raw_object);
}

// this is our custom .toString implementation for wrapper objects
// we want to masquerade wrappers of Python callables as [object Function]
// this is to satisfy some tests of original STPyV8 implementation which had
// two distinct code paths for creating wrappers depending on isPythonCallable
void toStringImpl(const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  auto v8_isolate = v8_info.GetIsolate();
  auto raw_wrapped_object = lookupTracedObject(v8_info.Holder());
  assert(raw_wrapped_object);
  TRACE("toStringImpl v8_info={} raw_wrapped_object={}", v8_info, S$(raw_wrapped_object));

  // note this call does not require GIL
  if (isPythonCallable(raw_wrapped_object)) {
    // "fake it until you make it"
    auto v8_result = v8::String::NewFromUtf8(v8_isolate, "Function").ToLocalChecked();
    v8_info.GetReturnValue().Set(v8_result);
    return;
  }

  // undefined value causes JS runtime to fall back to default implementation
  v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
}

void CPythonObject::NamedGetter(v8::Local<v8::Name> v8_name, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::NamedGetter v8_name={} v8_info={}", v8_name, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  if (v8_name->IsSymbol()) {
    if (v8_name->StrictEquals(v8::Symbol::GetToStringTag(v8_isolate))) {
      toStringImpl(v8_info);
      return;
    }

    // ignore other symbols for now, see https://github.com/area1/stpyv8/issues/8
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_result = withPythonErrorInterception(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = wrap(v8_isolate, v8_info.Holder());
    auto v8_utf_name = v8u::toUTF(v8_isolate, v8_name.As<v8::String>());

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
          return wrap(py_result);
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

    return wrap(py_val);
  });

  auto v8_final_result = VALUE_OR_LAZY(v8_result, v8::Undefined(v8_isolate));
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
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_result = withPythonErrorInterception(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = wrap(v8_isolate, v8_info.Holder());
    v8::String::Utf8Value v8_utf_name(v8_isolate, v8_name);
    auto py_val = wrap(v8_isolate, v8_value);
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

  auto v8_final_result = VALUE_OR_LAZY(v8_result, v8::Undefined(v8_isolate));
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
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_result = withPythonErrorInterception(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = wrap(v8_isolate, v8_info.Holder());
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

  auto v8_final_result = VALUE_OR_LAZY(v8_result, v8::Local<v8::Integer>());
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
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_result = withPythonErrorInterception(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = wrap(v8_isolate, v8_info.Holder());
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

  auto v8_final_result = VALUE_OR_LAZY(v8_result, v8::Local<v8::Boolean>());
  v8_info.GetReturnValue().Set(v8_final_result);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-lambda-function-name"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

void CPythonObject::NamedEnumerator(const v8::PropertyCallbackInfo<v8::Array>& v8_info) {
  TRACE("CPythonObject::NamedEnumerator v8_info={}", v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);

  auto v8_result = withPythonErrorInterception(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = wrap(v8_isolate, v8_info.Holder());
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

      auto py_item = wrap(py::reinterpret_borrow<py::object>(raw_item));
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      auto v8_context = v8u::getCurrentContext(v8_isolate);
      auto res = v8_array->Set(v8_context, v8_i, py_item);
      res.Check();
    }
    return v8_array;
  });

  auto v8_final_result = VALUE_OR_LAZY(v8_result, v8::Local<v8::Array>());
  v8_info.GetReturnValue().Set(v8_final_result);
}

#pragma clang diagnostic pop
#pragma clang diagnostic pop
