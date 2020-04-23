#pragma once

#include "Base.h"

template <typename F>
auto withAllowedPythonThreads(F&& fn) {
  // https://docs.python.org/3/c-api/init.html#releasing-the-gil-from-extension-code
  // this is an equivalent of:
  //   Py_BEGIN_ALLOW_THREADS
  //     ... Do some blocking I/O operation ...
  //   Py_END_ALLOW_THREADS
  auto raw_thread_state = PyEval_SaveThread();
  [[maybe_unused]] auto&& _ = finally([&]() noexcept { PyEval_RestoreThread(raw_thread_state); });
  return fn();
}