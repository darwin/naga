#pragma once

#include "Base.h"
#include "JSObject.h"

template <typename T>
const void* voidThis(const T* v) {
  return reinterpret_cast<const void*>(v);
}

#define THIS voidThis(this)

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& obj);
std::ostream& operator<<(std::ostream& os, const CJSException& ex);
std::ostream& operator<<(std::ostream& os, const CJSObject& obj);
std::ostream& operator<<(std::ostream& os, const CJSObjectAPI& obj);
std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj);
std::ostream& operator<<(std::ostream& os, const CContext& obj);
std::ostream& operator<<(std::ostream& os, const CEngine& obj);
std::ostream& operator<<(std::ostream& os, const CScript& obj);
std::ostream& operator<<(std::ostream& os, const CJSStackFrame& obj);

// https://fmt.dev/latest/api.html#formatting-user-defined-types
// warning! operator<< is tricky with namespaces, it must be implemented inside, not in global scope
// see https://github.com/fmtlib/fmt/issues/1542#issuecomment-581855567
namespace v8 {

std::ostream& operator<<(std::ostream& os, const TryCatch& v8_val);

std::ostream& operator<<(std::ostream& os, const Local<Private>& v8_val);
std::ostream& operator<<(std::ostream& os, const Local<Context>& v8_val);
std::ostream& operator<<(std::ostream& os, const Local<Script>& v8_val);
std::ostream& operator<<(std::ostream& os, const Local<ObjectTemplate>& v8_val);
std::ostream& operator<<(std::ostream& os, const Local<Message>& v8_val);
std::ostream& operator<<(std::ostream& os, const Local<StackFrame>& v8_val);
std::ostream& operator<<(std::ostream& os, const Local<StackTrace>& v8_val);

std::ostream& printLocal(std::ostream& os, const Local<Value>& v8_val);

template <typename T, typename = typename std::enable_if_t<std::is_base_of_v<Value, T>>>
std::ostream& operator<<(std::ostream& os, const Local<T>& v) {
  os << "v8::Local<";
  printLocal(os, v);
  os << ">";
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Eternal<T>& v) {
  return os << "v8::Eternal<" << v.Get(v8u::getCurrentIsolate()) << ">";
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

template <>
struct fmt::formatter<py::error_already_set> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const py::error_already_set& val, FormatContext& ctx) {
    return format_to(ctx.out(), "py::ex[type={} what={}]", val.type(), val.what());
  }
};

template <>
struct fmt::formatter<py::handle> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const py::handle& val, FormatContext& ctx) {
    auto raw_obj = val.ptr();
    if (!raw_obj) {
      return format_to(ctx.out(), "py::object 0x0");
    } else {
      auto py_repr = py::repr(val);
      return format_to(ctx.out(), "py::object {} {}", static_cast<void*>(raw_obj), py_repr);
    }
  }
};

// see https://github.com/fmtlib/fmt/issues/1621
struct wstring_printer {
  const std::wstring& m_ws;
};

template <>
struct fmt::formatter<wstring_printer> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const wstring_printer& val, FormatContext& ctx) {
    try {
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
      auto utf8_string = converter.to_bytes(val.m_ws);
      return format_to(ctx.out(), "{}", utf8_string);
    } catch (...) {
      return format_to(ctx.out(), "?wstring?");
    }
  }
};

struct isolateref_printer {
  const v8::IsolateRef& m_isolate;
};

template <>
struct fmt::formatter<isolateref_printer> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const isolateref_printer& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8::IsolateRef {}", static_cast<void*>(val.m_isolate));
  }
};

struct raw_object_printer {
  PyObject* m_raw_object;
};

template <>
struct fmt::formatter<raw_object_printer> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const raw_object_printer& val, FormatContext& ctx) {
    return format_to(ctx.out(), "PyObject {}", static_cast<void*>(val.m_raw_object));
  }
};

struct roles_printer {
  CJSObject::Roles m_roles;
};

template <>
struct fmt::formatter<roles_printer> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const roles_printer& val, FormatContext& ctx) {
    std::vector<const char*> flags;
    if ((val.m_roles & CJSObject::Roles::Function) == CJSObject::Roles::Function) {
      flags.push_back("Function");
    }
    if ((val.m_roles & CJSObject::Roles::Array) == CJSObject::Roles::Array) {
      flags.push_back("Array");
    }
    return format_to(ctx.out(), "[{}]", fmt::join(flags, ","));
  }
};
