#pragma once

#include "Base.h"

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& obj);
std::ostream& operator<<(std::ostream& os, const CJSException& ex);
std::ostream& operator<<(std::ostream& os, const CJSObject& obj);
std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj);

std::ostream& operator<<(std::ostream& os, v8::Local<v8::Value> v8_val);

// https://fmt.dev/latest/api.html#formatting-user-defined-types
// I wasn't able to get above operator<< functions to work with spdlog/fmt lib out-of-the box
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