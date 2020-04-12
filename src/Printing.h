#pragma once

#include "Base.h"

#define THIS voidThis(this)

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& obj);
std::ostream& operator<<(std::ostream& os, const CJSException& ex);
std::ostream& operator<<(std::ostream& os, const CJSObject& obj);
std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj);
std::ostream& operator<<(std::ostream& os, const CContext& obj);
std::ostream& operator<<(std::ostream& os, const CEngine& obj);
std::ostream& operator<<(std::ostream& os, const CScript& obj);
std::ostream& operator<<(std::ostream& os, const CJSStackFrame& obj);

std::ostream& operator<<(std::ostream& os, v8::Local<v8::Value> v8_val);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-readability-casting"
template <typename T>
void* voidThis(const T* v) {
  return (void*)v;
}
#pragma clang diagnostic pop

// https://fmt.dev/latest/api.html#formatting-user-defined-types
// I wasn't able to get above operator<< functions to work with spdlog/fmt lib out-of-the box
// maybe this was the issue? https://github.com/fmtlib/fmt/issues/1542#issuecomment-581855567
// this is an alternative way when we defined custom formatter and delegate work to operator<< explicitly
// I'm not sure about codegen complexity, but we use logging only during development so this is ok, I guess
template <typename T>
struct fmt::formatter<v8::Local<T>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<T>& val, FormatContext& ctx) {
    std::ostringstream msg;
    msg << val;
    return format_to(ctx.out(), "{}", msg.str());
  }
};

template <>
struct fmt::formatter<v8::Local<v8::Context>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<v8::Context>& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8::Context {} global={}", static_cast<void*>(*val), val->Global());
  }
};

template <>
struct fmt::formatter<v8::Local<v8::Script>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<v8::Script>& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8::Script {}", static_cast<void*>(*val));
  }
};

template <>
struct fmt::formatter<v8::TryCatch> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::TryCatch& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8::TryCatch Message=[{}]", val.Message());
  }
};

template <>
struct fmt::formatter<v8::Local<v8::Message>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<v8::Message>& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8::Message {} '{}'", static_cast<void*>(*val), val->Get());
  }
};

template <>
struct fmt::formatter<v8::Local<v8::StackFrame>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<v8::StackFrame>& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8::StackFrame {} ScriptId={} Script={}", static_cast<void*>(*val), val->GetScriptId(),
                     val->GetScriptNameOrSourceURL());
  }
};

template <>
struct fmt::formatter<v8::Local<v8::StackTrace>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<v8::StackTrace>& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8::StackTrace {} FrameCount={}", static_cast<void*>(*val), val->GetFrameCount());
  }
};

template <typename T>
struct fmt::formatter<v8::PropertyCallbackInfo<T>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::PropertyCallbackInfo<T>& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8:PCI[This={} Holder={} ReturnValue={}]", val.This(), val.Holder(),
                     val.GetReturnValue().Get());
  }
};

template <typename T>
struct fmt::formatter<v8::FunctionCallbackInfo<T>> {
  [[maybe_unused]] static constexpr auto parse(const format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::FunctionCallbackInfo<T>& val, FormatContext& ctx) {
    return format_to(ctx.out(), "v8:FCI[Length={} This={} Holder={} ReturnValue={}]", val.Length(), val.This(),
                     val.Holder(), val.GetReturnValue().Get());
  }
};

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
