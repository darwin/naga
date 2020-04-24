#pragma once

#include "Base.h"

namespace v8u {

v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const char* s);
v8::Local<v8::String> toString(v8::IsolatePtr v8_isolate, const std::string& str);
v8::Local<v8::String> toString(const std::string& str);
v8::Local<v8::String> toString(const std::wstring& str);
v8::Local<v8::String> toString(const py::handle& py_str);

v8::Local<v8::Integer> toPositiveInteger(v8::IsolatePtr v8_isolate, int i);

v8::String::Utf8Value toUTF(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_value);
std::string toStdString(v8::IsolatePtr v8_isolate, v8::Local<v8::Value> v8_value);

void checkContext(v8::IsolatePtr v8_isolate);

v8::IsolatePtr getCurrentIsolate();
v8::Context::Scope withContext(v8::Local<v8::Context> v8_context);
v8::HandleScope withScope(v8::IsolatePtr v8_isolate);
v8::EscapableHandleScope withEscapableScope(v8::IsolatePtr v8_isolate);
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

// an alternative for withTryCatch, which does an automatic check for exceptions at the end of scope
// one can still use manual checkTryCatch for ad-hoc checks sooner
class AutoTryCatch : public v8::TryCatch {
  v8::IsolatePtr m_v8_isolate;
  decltype(std::uncaught_exceptions()) m_recorded_uncaught_exceptions;

 public:
  explicit AutoTryCatch(v8::IsolatePtr v8_isolate)
      : v8::TryCatch(v8_isolate), m_v8_isolate(v8_isolate), m_recorded_uncaught_exceptions(std::uncaught_exceptions()) {
    HTRACE(kAutoTryCatchLogger, "AutoTryCatch {");
    LOGGER_INDENT_INCREASE;
  }
  // throwing in a destructor is dangerous, please read this: https://stackoverflow.com/a/4098662/84283
  // we have to explicitly add `noexcept(false)` otherwise bad bad things will happen:
  // https://stackoverflow.com/a/41429901/84283
  ~AutoTryCatch() noexcept(false) {
    LOGGER_INDENT_DECREASE;
    HTRACE(kAutoTryCatchLogger, "} ~AutoTryCatch");
    bool non_exceptional_scope_unwinding = m_recorded_uncaught_exceptions == std::uncaught_exceptions();
    // we have to be careful and don't cause a new exception if this destructor happens to be called
    // during uncaught exception stack unwinding
    if (non_exceptional_scope_unwinding) {
      checkTryCatch(m_v8_isolate, this);
    }
  }
};

inline AutoTryCatch withAutoTryCatch(v8::IsolatePtr v8_isolate) {
  return AutoTryCatch{v8_isolate};
}

}  // namespace v8u