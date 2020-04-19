#pragma once

#include "Base.h"

template <typename F>
auto withPythonAllowThreadsGuard(F&& fn) {
  PyThreadState* raw_thread_state = PyEval_SaveThread();
  [[maybe_unused]] auto _ = gsl::finally([&]() { PyEval_RestoreThread(raw_thread_state); });
  return fn();
}