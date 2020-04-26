#ifndef NAGA_PYTHONEXCEPTIONS_H_
#define NAGA_PYTHONEXCEPTIONS_H_

#include "Base.h"
#include "PythonObject.h"

template <typename F>
auto withPythonErrorInterception(v8::IsolatePtr v8_isolate, F&& fn) {
  try {
    return std::optional(fn());
  } catch (const py::error_already_set& e) {
    CPythonObject::ThrowJSException(v8_isolate, e);
  } catch (const std::exception& e) {
    auto v8_msg = v8u::toString(v8_isolate, e.what());
    v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
  } catch (...) {
    auto v8_msg = v8u::toString(v8_isolate, "unknown exception when crossing boundary into Python");
    v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
  }
  // return empty value because of exception
  // std::nullopt_t doesn't work with type checking
  // we explicitly return std::optional<return type of F>()
  return decltype(std::optional(fn()))();
}

#endif