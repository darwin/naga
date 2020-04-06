#pragma once

#include "Base.h"

namespace v8u {

v8::Local<v8::String> toString(const std::string& str);
v8::Local<v8::String> toString(const std::wstring& str);
//v8::Local<v8::String> toString(py::object str);
v8::Local<v8::String> toString(pb::handle str);

v8::String::Utf8Value toUtf8Value(v8::Isolate* v8_isolate, v8::Local<v8::String> v8_string);

void checkContext(v8::Isolate* v8_isolate);
bool executionTerminating(v8::Isolate* v8_isolate);

v8::HandleScope getScope(v8::Isolate* v8_isolate);
v8::EscapableHandleScope openEscapableScope(v8::Isolate* v8_isolate);
v8::TryCatch openTryCatch(v8::Isolate* v8_isolate);

}  // namespace v8u

std::ostream& operator<<(std::ostream& os, v8::Local<v8::Value> v8_val);