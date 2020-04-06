#pragma once

#include "Base.h"
#include "Exception.h"

enum IsolateDataSlot { kReserved = 0, kJSObjectTemplate = 1 };

class CIsolate;

typedef std::shared_ptr<CIsolate> CIsolatePtr;

class CIsolate {
  v8::Isolate* m_v8_isolate;
  bool m_owner;
  
  void Init(bool owner);

 public:
  CIsolate();
  CIsolate(bool owner);
  CIsolate(v8::Isolate* v8_isolate);
  ~CIsolate();

  v8::Isolate* GetIsolate();

  CJavascriptStackTracePtr GetCurrentStackTrace(int frame_limit, v8::StackTrace::StackTraceOptions v8_options);

  static pb::object GetCurrent();

  void Enter() { m_v8_isolate->Enter(); }
  void Leave() { m_v8_isolate->Exit(); }
  void Dispose() { m_v8_isolate->Dispose(); }

  bool IsLocked() { return v8::Locker::IsLocked(m_v8_isolate); }

  static void Expose(pb::module& m);
};
