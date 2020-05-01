#ifndef NAGA_PYBINDLOGGING_H_
#define NAGA_PYBINDLOGGING_H_

#include "Base.h"
#include "Meta.h"
#include "RawUtils.h"

namespace pybind11 {

std::string traceExposing(const char* name, const char* prefix = nullptr, const char* postfix = nullptr) {
  std::stringstream ss;
  if (prefix) {
    ss << prefix << ".";
  }
  ss << name;
  if (postfix) {
    ss << " " << postfix;
  }
  auto full_name = ss.str();
  HTRACE(kPythonExposeLogger, "exposing {}", full_name);
  return full_name;
}

template <typename Functor, typename GuardFactory, typename FunctorSignature>
struct expose_wrapper {};

template <typename Functor, typename GuardFactory, typename Return, typename... Args>
struct expose_wrapper<Functor, GuardFactory, Return(Args...)> {
  Functor m_functor;
  GuardFactory m_guard_factory;

  auto operator()(Args... args) const {
    [[maybe_unused]] auto guard = m_guard_factory();
    return m_functor(args...);
  }
};

template <typename T, typename GuardFactory>
auto make_expose_wrapper(T&& f, GuardFactory guard_factory) {
  auto functor = to_functor(std::forward<T>(f));
  using Functor = decltype(functor);
  return expose_wrapper<Functor, GuardFactory, typename callable_signature<Functor>::type>{functor, guard_factory};
}

template <typename Func>
auto wrapWithLogger(Func&& f, const char* name, const char* prefix = nullptr, const char* postfix = nullptr) {
  auto full_name = traceExposing(name, prefix, postfix);
  return make_expose_wrapper(std::forward<Func>(f), [full_name] { return JSLandLogger(full_name.c_str()); });
}

template <typename Func>
auto wrapWithLogger(Func&& f, const char* name, PyTypeObject* raw_type, const char* postfix = "") {
  auto prefix = pythonTypeName(raw_type);
  return wrapWithLogger(std::forward<Func>(f), name, prefix, postfix);
}

}  // namespace pybind11

#endif