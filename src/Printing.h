#pragma once

#include "Base.h"

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& obj);
std::ostream& operator<<(std::ostream& os, const CJSException& ex);
std::ostream& operator<<(std::ostream& os, const CJSObject& obj);
std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj);

std::ostream& operator<<(std::ostream& os, v8::Local<v8::Value> v8_val);

// https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<v8::Local<v8::Name>> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<v8::Name>& d, FormatContext& ctx) {
    std::ostringstream msg;
    msg << d;
    return format_to(ctx.out(), "{}", msg.str());
  }
};

// TODO: figure out how to do this via template
// hint: see "You can also write a formatter for a hierarchy of classes" via the above link
template <>
struct fmt::formatter<v8::Local<v8::Value>> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const v8::Local<v8::Value>& d, FormatContext& ctx) {
    std::ostringstream msg;
    msg << d;
    return format_to(ctx.out(), "{}", msg.str());
  }
};
