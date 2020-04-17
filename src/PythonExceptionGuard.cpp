#include "PythonExceptionGuard.h"

void withPythonExceptionGuard(const v8::IsolateRef& v8_isolate, std::function<void()> fn) {
  withPythonExceptionGuard<bool>(v8_isolate, [&fn]() {
    fn();
    return true;
  });
}