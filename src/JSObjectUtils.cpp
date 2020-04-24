#include "JSObjectUtils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

bool isCLJSType(v8::Local<v8::Object> v8_obj) {
  if (v8_obj.IsEmpty()) {
    return false;
  }

  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  // TODO: optimize this
  auto v8_ctor_key = v8::String::NewFromUtf8(v8_isolate, "constructor").ToLocalChecked();
  auto v8_ctor_val = v8_obj->Get(v8_context, v8_ctor_key).ToLocalChecked();

  if (v8_ctor_val.IsEmpty() || !v8_ctor_val->IsObject()) {
    return false;
  }

  auto v8_ctor_obj = v8::Local<v8::Object>::Cast(v8_ctor_val);
  auto v8_cljs_key = v8::String::NewFromUtf8(v8_isolate, "cljs$lang$type").ToLocalChecked();
  auto v8_cljs_val = v8_ctor_obj->Get(v8_context, v8_cljs_key).ToLocalChecked();

  return !(v8_cljs_val.IsEmpty() || !v8_cljs_val->IsBoolean());

  // note:
  // we should cast cljs_val to object and check cljs_obj->BooleanValue()
  // but we assume existence of property means true value
}