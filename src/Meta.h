#ifndef NAGA_META_H_
#define NAGA_META_H_

#include "Base.h"

// Here we stash away our helpers for template meta programming
//
// WARNING:
//
// !!! A DANGER OF BRAIN DAMAGE !!!
//
// [ continue at your own risk ]

// ForwardTo is a helper class which redirects function calls to instance calls
// The convention is that it expects first function parameter to be the instance pointer.
// The template machinery bellow then perfectly forwards the call to instance.
//
// in Python: toolkit.something(instance, arg1, arg2, ...)
// calls (via pybind) our wrapper's operator(), and we in turn call
// instance->Something(arg1, arg2, ...)
// credit: https://stackoverflow.com/a/46533854/84283
template <auto F>
struct ForwardTo {};

template <typename T, typename... Args, auto (T::*F)(Args...)>
struct ForwardTo<F> {
  auto operator()(const CJSObjectPtr& obj, Args&&... args) const {
    return std::invoke(F, *obj, std::forward<Args>(args)...);
  }
};

// this variant differs only that it deals with const member functions
// I don't know how to share the code with a single template
template <typename T, typename... Args, auto (T::*F)(Args...) const>
struct ForwardTo<F> {
  auto operator()(const CJSObjectPtr& obj, Args&&... args) const {
    return std::invoke(F, *obj, std::forward<Args>(args)...);
  }
};

// `callable_signature<T>::type` gives us a signature of passed T, which can be:
// a) function (pointer)
// b) lambda/functor (with 'operator()')
// c) pointer to const member function
// d) pointer to member function

// https://stackoverflow.com/a/21665705/84283

// For generic types that are functors, delegate to its 'operator()'
template <typename T>
struct callable_signature : public callable_signature<decltype(&T::operator())> {};

// for pointers to const member function
template <typename ClassType, typename ReturnType, typename... Args>
struct callable_signature<ReturnType (ClassType::*)(Args...) const> {
  using type = ReturnType(Args...);
};

// for pointers to member function
template <typename ClassType, typename ReturnType, typename... Args>
struct callable_signature<ReturnType (ClassType::*)(Args...)> {
  using type = ReturnType(Args...);
};

// for function pointers
template <typename ReturnType, typename... Args>
struct callable_signature<ReturnType (*)(Args...)> {
  using type = ReturnType(Args...);
};

// to_functor takes some callable thing and gives us a functor wrapping it

template <typename L>
auto to_functor(L&& f) {
  return std::forward<L>(f);
}

template <typename Return, typename... Args>
auto to_functor(Return (*f)(Args...)) {
  return [f](Args... args) -> Return { return f(args...); };
}

template <typename Return, typename Class, typename... Args>
auto to_functor(Return (Class::*f)(Args...)) {
  return [f](Class* c, Args... args) -> Return { return (c->*f)(args...); };
}

template <typename Return, typename Class, typename... Args>
auto to_functor(Return (Class::*f)(Args...) const) {
  return [f](const Class* c, Args... args) -> Return { return (c->*f)(args...); };
}

#endif