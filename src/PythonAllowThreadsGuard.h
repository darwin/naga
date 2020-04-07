#pragma once

#include "Base.h"

template <typename T>
T withPythonAllowThreadsGuard(std::function<T()> fn) {
  PyThreadState* raw_thread_state = PyEval_SaveThread();
  [[maybe_unused]] auto _ = gsl::finally([&]() { PyEval_RestoreThread(raw_thread_state); });
  return fn();
}

void withPythonAllowThreadsGuard(std::function<void()> fn);