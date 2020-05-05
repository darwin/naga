#ifndef NAGA_V8UTILS_H_
#define NAGA_V8UTILS_H_

#include "Base.h"
#include "Logging.h"
#include "V8UtilsObservedHandleScope.h"
#include "V8UtilsAutoTryCatch.h"

namespace v8u {

v8::Local<v8::String> pythonBytesObjectToString(v8::IsolatePtr v8_isolate, PyObject* raw_bytes_obj);

v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const char* s);
v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const std::string& str);
v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const std::string_view& sv);
v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const std::wstring& str);
v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const py::handle& py_str);

v8::Local<v8::Integer> toPositiveInteger(v8::IsolatePtr v8_isolate, int i);

v8::String::Utf8Value toUTF(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_value);
std::string toStdString(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_value);

v8::IsolatePtr getCurrentIsolate();
v8::Isolate* getCurrentIsolateUnchecked();
v8::Local<v8::Context> getCurrentContext(v8::IsolatePtr v8_isolate);
v8::Local<v8::Context> getCurrentContextUnchecked(v8::IsolatePtr v8_isolate);
v8::Context::Scope withContext(v8::Local<v8::Context> v8_context);
ObservedHandleScope withScope(v8::IsolatePtr v8_isolate);
ObservedEscapableHandleScope withEscapableScope(v8::IsolatePtr v8_isolate);
bool hasScope(v8::IsolatePtr v8_isolate);
v8::TryCatch withTryCatch(v8::IsolatePtr v8_isolate);
void checkTryCatch(v8::IsolatePtr v8_isolate, v8::TryCatchPtr v8_try_catch);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-runtime-references"
inline void checkTryCatch(v8::IsolatePtr v8_isolate, v8::TryCatch& v8_try_catch) {
  checkTryCatch(v8_isolate, &v8_try_catch);
}
#pragma clang diagnostic pop
v8::IsolatePtr createIsolate();
v8::ScriptOrigin createScriptOrigin(v8::Local<v8::Value> v8_name,
                                    v8::Local<v8::Integer> v8_line,
                                    v8::Local<v8::Integer> v8_col);
v8::Eternal<v8::Private> createEternalPrivateAPI(v8::IsolatePtr v8_isolate, const char* name);

std::string getCurrentStackTrace(v8::IsolatePtr v8_isolate);

}  // namespace v8u

#endif