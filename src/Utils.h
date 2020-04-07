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

// https://stackoverflow.com/a/26221725/84283
template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
  if (size <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);  // We don't want the '\0' inside
}
