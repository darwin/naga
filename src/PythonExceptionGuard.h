#pragma once

#include "Base.h"
#include "PythonObject.h"

template <typename T>
std::optional<T> withPythonExceptionGuard(v8::Isolate* isolate, std::function<T()> fn) {
  try {
    return fn();
  } catch (const pb::error_already_set& e) {
    CPythonObject::ThrowIf(isolate, e);
  } catch (const std::exception& e) {
    auto msg = v8::String::NewFromUtf8(isolate, e.what()).ToLocalChecked();
    isolate->ThrowException(v8::Exception::Error(msg));
  } catch (...) {
    auto msg = v8::String::NewFromUtf8(isolate, "unknown exception").ToLocalChecked();
    isolate->ThrowException(v8::Exception::Error(msg));
  }
  return std::nullopt;
}

void withPythonExceptionGuard(v8::Isolate* isolate, std::function<void()> fn);