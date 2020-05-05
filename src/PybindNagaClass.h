#ifndef NAGA_PYBINDNAGACLASS_H_
#define NAGA_PYBINDNAGACLASS_H_

#include "Base.h"
#include "Meta.h"
#include "Logging.h"
#include "PybindLogging.h"

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

// note we want to do private inheritance here so we don't miss any usage and cover all active uses with our wrapper
template <typename type_, typename... options>
class naga_class : /* private */ class_<type_, options...> {
  using super = class_<type_, options...>;

 public:
  using type = typename super::type;
  using type_alias = typename super::type_alias;
  constexpr static bool has_alias = super::has_alias;

  template <typename... Extra>
  naga_class(handle scope, const char* name, Extra&&... extra) : super(scope, name, std::forward<Extra>(extra)...) {}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
  template <typename... Args, typename... Extra>
  naga_class& def_ctor(const detail::initimpl::constructor<Args...>& init, const Extra&... extra) {
    // will be traced indirectly by calling .def
    init.execute(*this, extra...);
    return *this;
  }
#pragma clang diagnostic pop

  template <typename Func, typename... Args>
  naga_class& def(const char* name, Func&& f, Args&&... args) {
    auto raw_type = reinterpret_cast<PyTypeObject*>(this->ptr());
    auto wf = wrapWithLogger(method_adaptor<type_>(std::forward<Func>(f)), name, raw_type, "(ctor)");
    super::def(name, wf, std::forward<Args>(args)...);
    return *this;
  }

  template <typename Func, typename... Args>
  naga_class& def_method(const char* name, Func&& f, Args&&... args) {
    auto raw_type = reinterpret_cast<PyTypeObject*>(this->ptr());
    auto wf = wrapWithLogger(method_adaptor<type_>(std::forward<Func>(f)), name, raw_type, "(method)");
    super::def(name, wf, std::forward<Args>(args)...);
    return *this;
  }

  template <typename Func, typename... Args>
  naga_class& def_method_s(const char* name, Func&& f, Args&&... args) {
    auto raw_type = reinterpret_cast<PyTypeObject*>(this->ptr());
    auto wf = wrapWithLogger(std::forward<Func>(f), name, raw_type, "(static method)");
    super::def_static(name, wf, std::forward<Args>(args)...);
    return *this;
  }

  template <typename Getter, typename Setter, typename... Extra>
  naga_class& def_property(const char* name, Getter&& fget, Setter&& fset, Extra&&... extra) {
    auto raw_type = reinterpret_cast<PyTypeObject*>(this->ptr());
    auto wfget = wrapWithLogger(method_adaptor<type_>(std::forward<Getter>(fget)), name, raw_type, "(property getter)");
    auto wfset = wrapWithLogger(method_adaptor<type_>(std::forward<Setter>(fset)), name, raw_type, "(property setter)");
    super::def_property(name, wfget, wfset, std::forward<Extra>(extra)...);
    return *this;
  }

  template <typename Getter, typename... Extra>
  naga_class& def_property_r(const char* name, Getter&& fget, Extra&&... extra) {
    auto raw_type = reinterpret_cast<PyTypeObject*>(this->ptr());
    auto wfget =
        wrapWithLogger(method_adaptor<type_>(std::forward<Getter>(fget)), name, raw_type, "(readonly property getter)");
    super::def_property_readonly(name, wfget, std::forward<Extra>(extra)...);
    return *this;
  }

  template <typename Getter, typename... Extra>
  naga_class& def_property_rs(const char* name, Getter&& fget, Extra&&... extra) {
    auto raw_type = reinterpret_cast<PyTypeObject*>(this->ptr());
    auto wfget = wrapWithLogger(method_adaptor<type_>(std::forward<Getter>(fget)), name, raw_type,
                                "(static readonly property getter)");
    super::def_property_readonly_static(name, wfget, std::forward<Extra>(extra)...);
    return *this;
  }
};

}  // namespace pybind11

#endif