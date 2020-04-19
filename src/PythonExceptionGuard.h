#pragma once

#include "Base.h"
#include "PythonObject.h"

template <typename F>
auto withPythonExceptionGuard(const v8::IsolateRef& v8_isolate, F&& fn) {
  try {
    return std::optional(fn());
  } catch (const py::error_already_set& e) {
    CPythonObject::ThrowIf(v8_isolate, e);
  } catch (const std::exception& e) {
    auto v8_msg = v8::String::NewFromUtf8(v8_isolate, e.what()).ToLocalChecked();
    v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
  } catch (...) {
    auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "unknown exception").ToLocalChecked();
    v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
  }
  // return empty value because of exception
  // std::nullopt_t doesn't work with type checking
  // we explicitly return std::optional<return type of F>()
  return decltype(std::optional(fn()))();
}