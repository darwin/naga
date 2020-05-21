#ifndef NAGA_V8UTILS_H_
#define NAGA_V8UTILS_H_

#include "Base.h"
#include "Logging.h"
#include "V8XObservedHandleScope.h"
#include "V8XAutoTryCatch.h"
#include "V8XLockedIsolate.h"
#include "V8XProtectedIsolate.h"

namespace v8x {

v8::Local<v8::String> pythonBytesObjectToString(LockedIsolatePtr& v8_isolate, PyObject* raw_bytes_obj);

v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const char* s);
v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const std::string& str);
v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const std::string_view& sv);
v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const std::wstring& str);
v8::Local<v8::String> toString(LockedIsolatePtr& v8_isolate, const py::handle& py_str);

v8::Eternal<v8::String> createEternalString(LockedIsolatePtr& v8_isolate, const char* s);

v8::Local<v8::Integer> toPositiveInteger(LockedIsolatePtr& v8_isolate, int i);

v8::String::Utf8Value toUTF(LockedIsolatePtr& v8_isolate, v8::Local<v8::Value> v8_value);
std::string toStdString(LockedIsolatePtr& v8_isolate, v8::Local<v8::Value> v8_value);

LockedIsolatePtr getCurrentIsolate();
v8::Isolate* getCurrentIsolateUnchecked();
v8::Local<v8::Context> getCurrentContext(LockedIsolatePtr& v8_isolate);
v8::Local<v8::Context> getCurrentContextUnchecked(LockedIsolatePtr& v8_isolate);
v8::Context::Scope withContext(v8::Local<v8::Context> v8_context);
ObservedHandleScope withScope(LockedIsolatePtr& v8_isolate);
ObservedEscapableHandleScope withEscapableScope(LockedIsolatePtr& v8_isolate);
bool hasScope(LockedIsolatePtr& v8_isolate);
v8::TryCatch withTryCatch(LockedIsolatePtr& v8_isolate);
void checkTryCatch(LockedIsolatePtr& v8_isolate, TryCatchPtr v8_try_catch);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-runtime-references"
inline void checkTryCatch(LockedIsolatePtr& v8_isolate, v8::TryCatch& v8_try_catch) {
  checkTryCatch(v8_isolate, &v8_try_catch);
}
#pragma clang diagnostic pop
inline AutoTryCatch withAutoTryCatch(LockedIsolatePtr& v8_isolate) {
  return AutoTryCatch{v8_isolate};
}
ProtectedIsolatePtr createIsolate();
v8::ScriptOrigin createScriptOrigin(v8::Local<v8::Value> v8_name,
                                    v8::Local<v8::Integer> v8_line,
                                    v8::Local<v8::Integer> v8_col);
v8::Eternal<v8::Private> createEternalPrivateAPI(LockedIsolatePtr& v8_isolate, const char* name);

LockedIsolatePtr lockIsolate(v8::Isolate* v8_isolate);

}  // namespace v8x

#endif