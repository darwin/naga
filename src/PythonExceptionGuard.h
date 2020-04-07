#pragma once

#include "Base.h"
#include "PythonObject.h"

template <typename T>
std::optional<T> withPythonExceptionGuard(v8::IsolateRef v8_isolate, std::function<T()> fn) {
  try {
    return fn();
  } catch (const py::error_already_set& e) {
    CPythonObject::ThrowIf(v8_isolate, e);
  } catch (const std::exception& e) {
    auto v8_msg = v8::String::NewFromUtf8(v8_isolate, e.what()).ToLocalChecked();
    v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
  } catch (...) {
    auto v8_msg = v8::String::NewFromUtf8(v8_isolate, "unknown exception").ToLocalChecked();
    v8_isolate->ThrowException(v8::Exception::Error(v8_msg));
  }
  return std::nullopt;
}

void withPythonExceptionGuard(v8::IsolateRef v8_isolate, std::function<void()> fn);