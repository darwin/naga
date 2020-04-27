#ifndef NAGA_PYBINDNAGACLASS_H_
#define NAGA_PYBINDNAGACLASS_H_

#include "Base.h"
#include "Meta.h"
#include "Logging.h"

namespace pybind11 {

// Why is this terribly complicated code here?
// Well, if you have an idea how to implement the goal below in a better way I'm all ears
//
// The goal is to get something like a call guard but more flexible
// https://pybind11.readthedocs.io/en/stable/advanced/functions.html?highlight=guard#call-guard
//
// The problem is that we need a specific call guard per Python method. In other words we
// want a stateful guard. Unfortunately there does not seem to be a way how provide something
// like a "guard factory" to pybind11 where pybind would call it to construct guard instances during each call.
// It is only able to default-construct the guard type it was given without any state.
//
// I decided to wrap class methods with my own lambda where I implemented a guard factory.
// It is quite easy to do by hand by changing individual methods as it is suggested in the docs, but it is
// pretty hard to do it in an automated way. The main complexity is in handling different allowed callable types
// e.g. passed callable could be raw function pointer, member function pointer or something like lambda.
//
// In general:
// 1. We introduce naga_class inheriting class_ functionality where we override def-family of methods
// 2. In a def method when passed a callable, we wrap it using expose_wrapper
//
// Notes:
// 1. It is important to call method_adaptor on the input, so this casting is not lost with our wrapping
// 2. make_expose_wrapper first converts the callable to a functor (to get unified "type shape" which is easier to wrap)
// 3. operator() in expose_wrapper makes use of the passed guard factory before invoking the call
//
// TODO: this should be only used in dev builds, make sure this gets properly elided in release builds

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

// note we want to do private inheritance here so we don't miss any usage and cover all active uses with our wrapper
template <typename type_, typename... options>
class naga_class : /* private */ class_<type_, options...> {
  using super = class_<type_, options...>;

  const char* type_name() const { return reinterpret_cast<PyTypeObject*>(this->ptr())->tp_name; }

  std::string logName(const char* name, const char* postfix = nullptr) {
    if (postfix) {
      return fmt::format("{}.{} {}", type_name(), name, postfix);
    } else {
      return fmt::format("{}.{}", type_name(), name);
    }
  }

  std::string traceExposing(const char* name, const char* postfix = nullptr) {
    auto full_name = logName(name, postfix);
    HTRACE(kPythonExposeLogger, "exposing {}", full_name);
    return full_name;
  }

  template <typename Func>
  auto wrap(Func&& f, const char* name, const char* postfix = "") {
    auto full_name = traceExposing(name, postfix);
    return make_expose_wrapper(method_adaptor<type_>(std::forward<Func>(f)),
                               [full_name] { return JSLandLogger(full_name.c_str()); });
  }

 public:
  template <typename... Extra>
  naga_class(handle scope, const char* name, Extra&&... extra) : super(scope, name, std::forward<Extra>(extra)...) {}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
  template <typename... Args, typename... Extra>
  naga_class& def(const detail::initimpl::constructor<Args...>& init, const Extra&... extra) {
    traceExposing("constructor");
    super::def(init, extra...);
    return *this;
  }
#pragma clang diagnostic pop

  template <typename Func, typename... Args>
  naga_class& def(const char* name, Func&& f, Args&&... args) {
    auto wf = wrap(std::forward<Func>(f), name);
    super::def(name, wf, std::forward<Args>(args)...);
    return *this;
  }

  template <typename Func, typename... Args>
  naga_class& def_static(const char* name, Func&& f, Args&&... args) {
    auto wf = wrap(std::forward<Func>(f), name, "(static)");
    super::def_static(name, wf, std::forward<Args>(args)...);
    return *this;
  }

  template <typename Func, typename... Args>
  naga_class& def_property(const char* name, Func&& f, Args&&... args) {
    auto wf = wrap(std::forward<Func>(f), name, "(property)");
    super::def_property(name, wf, std::forward<Args>(args)...);
    return *this;
  }

  template <typename Func, typename... Args>
  naga_class& def_property_readonly(const char* name, Func&& f, Args&&... args) {
    auto wf = wrap(std::forward<Func>(f), name, "(readonly property)");
    super::def_property_readonly(name, wf, std::forward<Args>(args)...);
    return *this;
  }

  template <typename Func, typename... Args>
  naga_class& def_property_readonly_static(const char* name, Func&& f, Args&&... args) {
    auto wf = wrap(std::forward<Func>(f), name, "(static readonly property)");
    super::def_property_readonly_static(name, wf, std::forward<Args>(args)...);
    return *this;
  }
};

}  // namespace pybind11

#endif