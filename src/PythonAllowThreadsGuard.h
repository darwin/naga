#pragma once

#include "Base.h"

template <typename T>
T withPythonAllowThreadsGuard(std::function<T()> fn) {
  PyThreadState* thread_state = PyEval_SaveThread();
  [[maybe_unused]] auto _ = finally([&]() { PyEval_RestoreThread(thread_state); });
  return fn();
}

void withPythonAllowThreadsGuard(std::function<void()> fn);