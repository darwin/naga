#pragma once

#include "Base.h"

namespace v8u {

v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const char* s);
v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const std::string& str);
v8::Local<v8::String> toString(const std::string& str);
v8::Local<v8::String> toString(const std::wstring& str);
v8::Local<v8::String> toString(const py::handle& py_str);

v8::Local<v8::Integer> toPositiveInteger(v8::IsolatePtr v8_isolate, int i);

v8::String::Utf8Value toUTF(v8::IsolatePtr v8_isolate, v8::Local<v8::String> v8_string);

void checkContext(v8::IsolatePtr v8_isolate);

v8::IsolatePtr getCurrentIsolate();
v8::Context::Scope withContext(v8::Local<v8::Context> v8_context);
v8::HandleScope withScope(v8::IsolatePtr v8_isolate);
v8::EscapableHandleScope withEscapableScope(v8::IsolatePtr v8_isolate);
v8::TryCatch withTryCatch(v8::IsolatePtr v8_isolate);
v8::IsolatePtr createIsolate();
v8::ScriptOrigin createScriptOrigin(v8::Local<v8::Value> v8_name,
                                    v8::Local<v8::Integer> v8_line,
                                    v8::Local<v8::Integer> v8_col);
v8::Eternal<v8::Private> createEternalPrivateAPI(v8::IsolatePtr v8_isolate, const char* name);

}  // namespace v8u