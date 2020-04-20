#pragma once

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