#pragma once

#include "Base.h"
#include "PythonObject.h"

template <typename T>
T withPythonExceptionGuard(v8::Isolate* isolate, T ex, std::function<T()> fn) {
  try {
    return fn();
  } catch (const std::exception& ex) {
    auto msg = v8::String::NewFromUtf8(isolate, ex.what()).ToLocalChecked();
    isolate->ThrowException(v8::Exception::Error(msg));
  } catch (const py::error_already_set&) {
    CPythonObject::ThrowIf(isolate);
  } catch (...) {
    auto msg = v8::String::NewFromUtf8(isolate, "unknown exception").ToLocalChecked();
    isolate->ThrowException(v8::Exception::Error(msg));
  }
  return ex;
}

template <typename T>
T withPythonExceptionGuard(v8::Isolate* isolate, std::function<T()> fn) {
  return withPythonExceptionGuard(isolate, T(), fn);
}

void withPythonExceptionGuard(v8::Isolate* isolate, std::function<void()> fn);