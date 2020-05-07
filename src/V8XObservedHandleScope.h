#ifndef NAGA_V8OBSERVERVEDHNADLESCOPE_H_
#define NAGA_V8OBSERVERVEDHNADLESCOPE_H_

#include "Base.h"
#include "Logging.h"
#include "V8XLockedIsolate.h"
#include "V8XProtectedIsolate.h"

namespace v8x {

class ObservedHandleScope : public v8::HandleScope {
  int m_start_num_handles;

 public:
  explicit ObservedHandleScope(LockedIsolatePtr& v8_isolate)
      : v8::HandleScope(v8_isolate),
        m_start_num_handles(v8::HandleScope::NumberOfHandles(v8_isolate)) {
    HTRACE(kHandleScopeLogger, "HandleScope {");
    LOGGER_INDENT_INCREASE;
    increaseCurrentHandleScopeLevel(v8_isolate);
  }
  ~ObservedHandleScope() {
    auto v8_isolate = ProtectedIsolatePtr(this->GetIsolate()).lock();
    decreaseCurrentHandleScopeLevel(v8_isolate);
    LOGGER_INDENT_DECREASE;
    HTRACE(kHandleScopeLogger, "}} ~HandleScope (releasing {} handles)", [&] {
      auto end_num_handles = v8::HandleScope::NumberOfHandles(v8_isolate);
      auto num_handles = end_num_handles - m_start_num_handles;
      return num_handles;
    }());
  }
};

class ObservedEscapableHandleScope : public v8::EscapableHandleScope {
  int m_start_num_handles;

 public:
  explicit ObservedEscapableHandleScope(LockedIsolatePtr& v8_isolate)
      : v8::EscapableHandleScope(v8_isolate),
        m_start_num_handles(v8::HandleScope::NumberOfHandles(v8_isolate)) {
    HTRACE(kHandleScopeLogger, "EscapableHandleScope {");
    LOGGER_INDENT_INCREASE;
    increaseCurrentHandleScopeLevel(v8_isolate);
  }
  ~ObservedEscapableHandleScope() {
    auto v8_isolate = ProtectedIsolatePtr(this->GetIsolate()).lock();
    decreaseCurrentHandleScopeLevel(v8_isolate);
    LOGGER_INDENT_DECREASE;
    HTRACE(kHandleScopeLogger, "}} ~EscapableHandleScope (releasing {} handles)", [&] {
      auto end_num_handles = v8::HandleScope::NumberOfHandles(v8_isolate);
      auto num_handles = end_num_handles - m_start_num_handles;
      return num_handles;
    }());
  }
};

}  // namespace v8x

#endif