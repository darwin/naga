#include "PythonExceptionGuard.h"

void withPythonExceptionGuard(v8::IsolateRef v8_isolate, std::function<void()> fn) {
  withPythonExceptionGuard<bool>(v8_isolate, [&fn]() {
    fn();
    return true;
  });
}