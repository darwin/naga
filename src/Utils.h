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

// https://stackoverflow.com/a/25510879/84283
template <typename F>
struct FinalAction {
  FinalAction(F f) : m_f{f} {}
  ~FinalAction() { m_f(); }

 private:
  F m_f;
};

template <typename F>
FinalAction<F> finally(F f) {
  return FinalAction<F>(f);
}