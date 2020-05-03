#ifndef NAGA_V8UTILSOBSERVERVEDHNADLESCOPE_H_
#define NAGA_V8UTILSOBSERVERVEDHNADLESCOPE_H_

#include "Base.h"
#include "Logging.h"

namespace v8u {

class ObservedHandleScope : public v8::HandleScope {
  int m_start_num_handles;

 public:
  explicit ObservedHandleScope(v8::IsolatePtr v8_isolate)
      : v8::HandleScope(v8_isolate), m_start_num_handles(v8::HandleScope::NumberOfHandles(v8_isolate)) {
    HTRACE(kHandleScopeLogger, "HandleScope {");
    LOGGER_INDENT_INCREASE;
    increaseCurrentHandleScopeLevel(v8_isolate);
  }
  ~ObservedHandleScope() {
    auto v8_isolate = this->GetIsolate();
    decreaseCurrentHandleScopeLevel(v8_isolate);
    LOGGER_INDENT_DECREASE;
    auto end_num_handles = v8::HandleScope::NumberOfHandles(v8_isolate);
    auto num_handles = end_num_handles - m_start_num_handles;
    HTRACE(kHandleScopeLogger, "}} ~HandleScope (releasing {} handles)", num_handles);
  }
};

class ObservedEscapableHandleScope : public v8::EscapableHandleScope {
  int m_start_num_handles;

 public:
  explicit ObservedEscapableHandleScope(v8::IsolatePtr v8_isolate)
      : v8::EscapableHandleScope(v8_isolate), m_start_num_handles(v8::HandleScope::NumberOfHandles(v8_isolate)) {
    HTRACE(kHandleScopeLogger, "EscapableHandleScope {");
    LOGGER_INDENT_INCREASE;
    increaseCurrentHandleScopeLevel(v8_isolate);
  }
  ~ObservedEscapableHandleScope() {
    auto v8_isolate = this->GetIsolate();
    decreaseCurrentHandleScopeLevel(v8_isolate);
    LOGGER_INDENT_DECREASE;
    auto end_num_handles = v8::HandleScope::NumberOfHandles(v8_isolate);
    auto num_handles = end_num_handles - m_start_num_handles;
    HTRACE(kHandleScopeLogger, "}} ~EscapableHandleScope (releasing {} handles)", num_handles);
  }
};

}  // namespace v8u

#endif