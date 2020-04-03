#include "PythonExceptionGuard.h"

void withPythonExceptionGuard(v8::Isolate* isolate, std::function<void()> fn) {
  withPythonExceptionGuard<bool>(isolate, [&fn]() {
    fn();
    return true;
  });
}