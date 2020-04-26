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

// https://stackoverflow.com/a/17356259/84283
template <typename t>
class sentry {
  t m_o;

 public:
  sentry(t in_o) : m_o(std::move(in_o)) {}

  sentry(sentry&&) = delete;
  sentry(sentry const&) = delete;

  ~sentry() noexcept {
    static_assert(noexcept(m_o()),
                  "Please check that the finally block cannot throw, "
                  "and mark the lambda as noexcept.");
    m_o();
  }
};

template <typename t>
sentry<t> finally(t o) {
  return {std::move(o)};
}

#endif