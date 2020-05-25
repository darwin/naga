#ifndef NAGA_UTILS_H_
#define NAGA_UTILS_H_

#include "Base.h"

// https://stackoverflow.com/a/51695657/84283
template <typename T, typename F>
T value_or(const std::optional<T>& opt, F fn) {
  if (opt) {
    return *opt;
  } else {
    return fn();
  }
}

#define VALUE_OR_LAZY(opt, lazy_expr) value_or((opt), [&]() { return (lazy_expr); })

#endif