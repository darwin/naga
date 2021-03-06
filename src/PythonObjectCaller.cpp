#include "PythonObject.h"
#include "PythonExceptions.h"
#include "JSTracer.h"
#include "Wrapping.h"
#include "Logging.h"
#include "PythonUtils.h"
#include "Utils.h"
#include "Printing.h"
#include "PybindExtensions.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

void PythonObject::CallWrapperAsFunction(const v8::FunctionCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::CallWrapperAsFunction v8_info={}", v8_info);
  // "this" should be some wrapper object wrapping some callable from python land
  auto v8_this = v8_info.This();

  // extract wrapped python object
  auto raw_object = lookupTracedObject(v8_this);
  assert(raw_object);

  // use it
  auto py_callable = py::reinterpret_borrow<py::object>(raw_object);
  CallPythonCallable(py_callable, v8_info);
}

void PythonObject::CallPythonCallable(const py::object& py_fn, const v8::FunctionCallbackInfo<v8::Value>& v8_info) {
  TRACE("CPythonObject::CallPythonCallable py_fn={} v8_info={}", S$(py_fn.ptr()), v8_info);
  assert(PyCallable_Check(py_fn.ptr()));
  auto v8_isolate = v8x::lockIsolate(v8_info.GetIsolate());
  auto v8_scope = v8x::withScope(v8_isolate);

  auto py_gil = pyu::withGIL();
  auto v8_result = withPythonErrorInterception(v8_isolate, [&] {
    return wrap([&]() {
      switch (v8_info.Length()) {
        case 0:                                        //
          return py_fn();                              //
        case 1:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]));  //
        case 2:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]));  //
        case 3:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]));  //
        case 4:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]),   //
                       wrap(v8_isolate, v8_info[3]));  //
        case 5:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]),   //
                       wrap(v8_isolate, v8_info[3]),   //
                       wrap(v8_isolate, v8_info[4]));  //
        case 6:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]),   //
                       wrap(v8_isolate, v8_info[3]),   //
                       wrap(v8_isolate, v8_info[4]),   //
                       wrap(v8_isolate, v8_info[5]));  //
        case 7:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]),   //
                       wrap(v8_isolate, v8_info[3]),   //
                       wrap(v8_isolate, v8_info[4]),   //
                       wrap(v8_isolate, v8_info[5]),   //
                       wrap(v8_isolate, v8_info[6]));  //
        case 8:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]),   //
                       wrap(v8_isolate, v8_info[3]),   //
                       wrap(v8_isolate, v8_info[4]),   //
                       wrap(v8_isolate, v8_info[5]),   //
                       wrap(v8_isolate, v8_info[6]),   //
                       wrap(v8_isolate, v8_info[7]));  //
        case 9:                                        //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]),   //
                       wrap(v8_isolate, v8_info[3]),   //
                       wrap(v8_isolate, v8_info[4]),   //
                       wrap(v8_isolate, v8_info[5]),   //
                       wrap(v8_isolate, v8_info[6]),   //
                       wrap(v8_isolate, v8_info[7]),   //
                       wrap(v8_isolate, v8_info[8]));  //
        case 10:                                       //
          return py_fn(wrap(v8_isolate, v8_info[0]),   //
                       wrap(v8_isolate, v8_info[1]),   //
                       wrap(v8_isolate, v8_info[2]),   //
                       wrap(v8_isolate, v8_info[3]),   //
                       wrap(v8_isolate, v8_info[4]),   //
                       wrap(v8_isolate, v8_info[5]),   //
                       wrap(v8_isolate, v8_info[6]),   //
                       wrap(v8_isolate, v8_info[7]),   //
                       wrap(v8_isolate, v8_info[8]),   //
                       wrap(v8_isolate, v8_info[9]));  //
        default:
          auto v8_msg = v8x::toString(v8_isolate, "too many arguments");
          v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
          return py::js_undefined().cast<py::object>();
      }
    }());
  });

  auto v8_final_result = VALUE_OR_LAZY(v8_result, v8::Undefined(v8_isolate));
  v8_info.GetReturnValue().Set(v8_final_result);
}