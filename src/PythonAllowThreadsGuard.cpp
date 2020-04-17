#include "PythonAllowThreadsGuard.h"

void withPythonAllowThreadsGuard(std::function<void()> fn) {
  withPythonAllowThreadsGuard<bool>([&fn]() {
    fn();
    return true;
  });
}