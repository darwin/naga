#pragma once

#include "Base.h"

namespace v8u {

v8::Local<v8::String> toString(const std::string& str);
v8::Local<v8::String> toString(const std::wstring& str);
v8::Local<v8::String> toString(py::handle py_str);

v8::String::Utf8Value toUTF(const v8::IsolateRef& v8_isolate, v8::Local<v8::String> v8_string);

void checkContext(const v8::IsolateRef& v8_isolate);
bool executionTerminating(const v8::IsolateRef& v8_isolate);

v8::IsolateRef getCurrentIsolate();
v8::Context::Scope withContext(v8::Local<v8::Context> v8_context);
v8::HandleScope withScope(const v8::IsolateRef& v8_isolate);
v8::EscapableHandleScope withEscapableScope(const v8::IsolateRef& v8_isolate);
v8::TryCatch withTryCatch(const v8::IsolateRef& v8_isolate);
v8::IsolateRef createIsolate();

}  // namespace v8u