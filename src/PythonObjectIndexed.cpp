#include "PythonObject.h"
#include "JSObject.h"
#include "PythonExceptionGuard.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

void CPythonObject::IndexedGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::IndexedGetter index={} v8_info={}", index, v8_info);
  auto v8_isolate = v8_info.GetIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = CJSObject::Wrap(v8_isolate, v8_info.Holder());

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
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Undefined(v8_isolate));
    return;
  }

  auto v8_result = withPythonExceptionGuard(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = CJSObject::Wrap(v8_isolate, v8_info.Holder());

    if (PySequence_Check(py_obj.ptr())) {
      if (PySequence_SetItem(py_obj.ptr(), index, CJSObject::Wrap(v8_isolate, v8_value).ptr()) < 0) {
        auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "fail to set indexed value").ToLocalChecked();
        auto v8_ex = v8::Exception::Error(v8_msg);
        v8_isolate->ThrowException(v8_ex);
      }
    } else if (PyMapping_Check(py_obj.ptr())) {
      char buf[65];

      snprintf(buf, sizeof(buf), "%d", index);

      if (PyMapping_SetItemString(py_obj.ptr(), buf, CJSObject::Wrap(v8_isolate, v8_value).ptr()) < 0) {
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
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Integer>());
    return;
  }

  auto v8_result = withPythonExceptionGuard(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = CJSObject::Wrap(v8_isolate, v8_info.Holder());

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
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Boolean>());
    return;
  }

  auto v8_result = withPythonExceptionGuard(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = CJSObject::Wrap(v8_isolate, v8_info.Holder());

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
  auto v8_scope = v8u::withScope(v8_isolate);

  if (v8u::executionTerminating(v8_isolate)) {
    v8_info.GetReturnValue().Set(v8::Local<v8::Array>());
    return;
  }

  auto v8_result = withPythonExceptionGuard(v8_isolate, [&]() {
    auto py_gil = pyu::withGIL();
    auto py_obj = CJSObject::Wrap(v8_isolate, v8_info.Holder());
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