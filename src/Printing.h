#ifndef NAGA_PRINTING_H_
#define NAGA_PRINTING_H_

#include "Base.h"
#include "V8XUtils.h"

template <typename T>
const void* voidThis(const T* v) {
  return reinterpret_cast<const void*>(v);
}

#define THIS voidThis(this)

// for types which we cannot easily create operator<< we use printCoerced, ideally via P$ macro
// see https://github.com/fmtlib/fmt/issues/1621
#define P$(...) printCoerced(__VA_ARGS__)
std::string printCoerced(const std::wstring& v);
std::string printCoerced(v8::Isolate* v);

template <typename T>
struct SafePrinter {
  T m_v;
};

#define S$(...) printSafe(__VA_ARGS__)
template <typename T>
SafePrinter<T> printSafe(T v) {
  return SafePrinter<T>{v};
}

std::ostream& operator<<(std::ostream& os, const JSStackTrace& v);
std::ostream& operator<<(std::ostream& os, const JSException& v);
std::ostream& operator<<(std::ostream& os, const JSObject& v);
std::ostream& operator<<(std::ostream& os, const JSObjectAPI& v);
std::ostream& operator<<(std::ostream& os, const JSContext& v);
std::ostream& operator<<(std::ostream& os, const JSEngine& v);
std::ostream& operator<<(std::ostream& os, const JSScript& v);
std::ostream& operator<<(std::ostream& os, const JSStackFrame& v);
std::ostream& operator<<(std::ostream& os, PyObject* v);
std::ostream& operator<<(std::ostream& os, PyTypeObject* v);
// use this when it is unsafe to ask the python object for its string representation
std::ostream& operator<<(std::ostream& os, const SafePrinter<PyObject*>& v);

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::shared_ptr<T>& v) {
  if (!v) {
    return os << "std::shared_ptr<{EMPTY}>";
  } else {
    return os << "std::shared_ptr " << fmt::format("{} <", reinterpret_cast<const void*>(v.get())) << *v << ">";
  }
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::unique_ptr<T>& v) {
  if (!v) {
    return os << "std::unique_ptr<{EMPTY}>";
  } else {
    return os << "std::unique_ptr " << fmt::format("{} <", reinterpret_cast<const void*>(v.get())) << *v << ">";
  }
}

// https://fmt.dev/latest/api.html#formatting-user-defined-types
// warning! operator<< is tricky with namespaces, it must be implemented inside, not in global scope
// see https://github.com/fmtlib/fmt/issues/1542#issuecomment-581855567
namespace v8 {

std::ostream& operator<<(std::ostream& os, const TryCatch& v);
std::ostream& operator<<(std::ostream& os, const Local<Private>& v);
std::ostream& operator<<(std::ostream& os, const Local<Context>& v);
std::ostream& operator<<(std::ostream& os, const Local<Script>& v);
std::ostream& operator<<(std::ostream& os, const Local<ObjectTemplate>& v);
std::ostream& operator<<(std::ostream& os, const Local<Message>& v);
std::ostream& operator<<(std::ostream& os, const Local<StackFrame>& v);
std::ostream& operator<<(std::ostream& os, const Local<StackTrace>& v);

std::ostream& printLocalValue(std::ostream& os, const Local<Value>& v);

template <typename T, typename = typename std::enable_if_t<std::is_base_of_v<Value, T>>>
std::ostream& operator<<(std::ostream& os, const Local<T>& v) {
  os << "v8::Local ";
  printLocalValue(os, v);
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Eternal<T>& v) {
  return os << "v8::Eternal<" << v.Get(v8x::getCurrentIsolate()) << ">";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, PropertyCallbackInfo<T> v) {
  return os << fmt::format("v8:PCI[This={} Holder={} ReturnValue={}]", v.This(), v.Holder(), v.GetReturnValue().Get());
}

template <typename T>
std::ostream& operator<<(std::ostream& os, FunctionCallbackInfo<T> v) {
  return os << fmt::format("v8:FCI[Length={} This={} Holder={} ReturnValue={}]", v.Length(), v.This(), v.Holder(),
                           v.GetReturnValue().Get());
}

}  // namespace v8

namespace v8x {

std::ostream& operator<<(std::ostream& os, const ProtectedIsolatePtr& v);

}  // namespace v8x

namespace pybind11 {

std::ostream& operator<<(std::ostream& os, const error_already_set& v);

}

#endif