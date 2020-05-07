#ifndef NAGA_V8UTILSAUTOTRYCATCH_H_
#define NAGA_V8UTILSAUTOTRYCATCH_H_

#include "Base.h"
#include "Logging.h"
#include "V8ProtectedIsolate.h"

namespace v8u {

void checkTryCatch(v8::LockedIsolatePtr& v8_isolate, v8::TryCatchPtr v8_try_catch);

// an alternative for withTryCatch, which does an automatic check for exceptions at the end of scope
// one can still use manual checkTryCatch for ad-hoc checks sooner
class AutoTryCatch : public v8::TryCatch {
  v8::ProtectedIsolatePtr m_v8_isolate;
  decltype(std::uncaught_exceptions()) m_recorded_uncaught_exceptions;

 public:
  explicit AutoTryCatch(v8::ProtectedIsolatePtr v8_protected_isolate)
      : v8::TryCatch(v8_protected_isolate.lock()),
        m_v8_isolate(v8_protected_isolate),
        m_recorded_uncaught_exceptions(std::uncaught_exceptions()) {
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
      auto v8_isolate = m_v8_isolate.lock();
      checkTryCatch(v8_isolate, this);
    }
  }
};

inline AutoTryCatch withAutoTryCatch(v8::LockedIsolatePtr& v8_isolate) {
  return AutoTryCatch{v8_isolate};
}

}  // namespace v8u

#endif