#include "PythonObject.h"
#include "JSObject.h"
#include "PythonExceptionGuard.h"

#define TRACE(...) (SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__))

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