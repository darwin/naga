#pragma once

#include "Base.h"

namespace v8u {

v8::Local<v8::String> toString(const std::string& str);
v8::Local<v8::String> toString(const std::wstring& str);
v8::Local<v8::String> toString(py::handle py_str);

v8::String::Utf8Value toUtf8Value(const v8::IsolateRef& v8_isolate, v8::Local<v8::String> v8_string);

void checkContext(const v8::IsolateRef& v8_isolate);
bool executionTerminating(const v8::IsolateRef& v8_isolate);

v8::IsolateRef getCurrentIsolate();
v8::HandleScope openScope(const v8::IsolateRef& v8_isolate);
v8::EscapableHandleScope openEscapableScope(const v8::IsolateRef& v8_isolate);
v8::TryCatch openTryCatch(const v8::IsolateRef& v8_isolate);
v8::IsolateRef createIsolate();

}  // namespace v8u